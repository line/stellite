// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copyright 2016 LINE Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "stellite/server/test_tools/simple_http_server.h"


#include <utility>

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/sys_byteorder.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "net/base/net_errors.h"
#include "net/http/http_chunked_decoder.h"
#include "net/server/http_connection.h"
#include "net/server/http_server_request_info.h"
#include "net/server/http_server_response_info.h"
#include "net/socket/server_socket.h"
#include "net/socket/stream_socket.h"
#include "net/socket/tcp_server_socket.h"

namespace net {

SimpleHttpServer::SimpleHttpServer(
    std::unique_ptr<ServerSocket> server_socket,
    SimpleHttpServer::Delegate* delegate)
    : server_socket_(std::move(server_socket)),
      delegate_(delegate),
      last_id_(0),
      weak_ptr_factory_(this) {
  DCHECK(server_socket_);
  // Start accepting connections in next run loop in case when delegate is not
  // ready to get callbacks.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&SimpleHttpServer::DoAcceptLoop,
                 weak_ptr_factory_.GetWeakPtr()));
}

SimpleHttpServer::~SimpleHttpServer() {
}

void SimpleHttpServer::SendRaw(int connection_id, const std::string& data) {
  HttpConnection* connection = FindConnection(connection_id);
  if (connection == NULL)
    return;

  bool writing_in_progress = !connection->write_buf()->IsEmpty();
  if (connection->write_buf()->Append(data) && !writing_in_progress)
    DoWriteLoop(connection);
}

void SimpleHttpServer::SendResponse(int connection_id,
                                    const HttpServerResponseInfo& response) {
  SendRaw(connection_id, response.Serialize());
}

void SimpleHttpServer::Send(int connection_id,
                      HttpStatusCode status_code,
                      const std::string& data,
                      const std::string& content_type) {
  HttpServerResponseInfo response(status_code);
  response.SetContentHeaders(data.size(), content_type);
  SendResponse(connection_id, response);
  SendRaw(connection_id, data);
}

void SimpleHttpServer::Send200(int connection_id,
                         const std::string& data,
                         const std::string& content_type) {
  Send(connection_id, HTTP_OK, data, content_type);
}

void SimpleHttpServer::Send404(int connection_id) {
  SendResponse(connection_id, HttpServerResponseInfo::CreateFor404());
}

void SimpleHttpServer::Send500(int connection_id, const std::string& message) {
  SendResponse(connection_id, HttpServerResponseInfo::CreateFor500(message));
}

void SimpleHttpServer::Close(int connection_id) {
  CloseConnection(connection_id);
  CloseRequest(connection_id);
  CloseDecoder(connection_id);
}

void SimpleHttpServer::CloseConnection(int connection_id) {
  auto it = id_to_connection_.find(connection_id);
  if (it == id_to_connection_.end())
    return;

  std::unique_ptr<HttpConnection> connection = std::move(it->second);
  id_to_connection_.erase(it);
  delegate_->OnClose(connection_id);


  // The call stack might have callbacks which still have the pointer of
  // connection. Instead of referencing connection with ID all the time,
  // destroys the connection in next run loop to make sure any pending
  // callbacks in the call stack return.
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE,
                                                  connection.release());

}

void SimpleHttpServer::CloseRequest(int connection_id) {
  auto it = id_to_request_.find(connection_id);
  if (it == id_to_request_.end()) {
    return;
  }

  std::unique_ptr<HttpServerRequestInfo> request = std::move(it->second);
  id_to_request_.erase(it);

  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE,
                                                  request.release());
}

void SimpleHttpServer::CloseDecoder(int connection_id) {
  auto it = id_to_decoder_.find(connection_id);
  if (it == id_to_decoder_.end()) {
    return;
  }

  std::unique_ptr<HttpChunkedDecoder> decoder = std::move(it->second);
  id_to_decoder_.erase(it);

  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE,
                                                  decoder.release());
}

int SimpleHttpServer::GetLocalAddress(IPEndPoint* address) {
  return server_socket_->GetLocalAddress(address);
}

void SimpleHttpServer::SetReceiveBufferSize(int connection_id, int32_t size) {
  HttpConnection* connection = FindConnection(connection_id);
  if (connection)
    connection->read_buf()->set_max_buffer_size(size);
}

void SimpleHttpServer::SetSendBufferSize(int connection_id, int32_t size) {
  HttpConnection* connection = FindConnection(connection_id);
  if (connection)
    connection->write_buf()->set_max_buffer_size(size);
}

void SimpleHttpServer::DoAcceptLoop() {
  int rv;
  do {
    rv = server_socket_->Accept(&accepted_socket_,
                                base::Bind(&SimpleHttpServer::OnAcceptCompleted,
                                           weak_ptr_factory_.GetWeakPtr()));
    if (rv == ERR_IO_PENDING)
      return;
    rv = HandleAcceptResult(rv);
  } while (rv == OK);
}

void SimpleHttpServer::OnAcceptCompleted(int rv) {
  if (HandleAcceptResult(rv) == OK)
    DoAcceptLoop();
}

int SimpleHttpServer::HandleAcceptResult(int rv) {
  if (rv < 0) {
    LOG(ERROR) << "Accept error: rv=" << rv;
    return rv;
  }

  std::unique_ptr<HttpConnection> connection_ptr =
      base::MakeUnique<HttpConnection>(++last_id_, std::move(accepted_socket_));
  HttpConnection* connection = connection_ptr.get();
  id_to_connection_[connection->id()] = std::move(connection_ptr);
  delegate_->OnConnect(connection->id());

  if (!HasClosedConnection(connection))
    DoReadLoop(connection);

  return OK;
}

void SimpleHttpServer::DoReadLoop(HttpConnection* connection) {
  int rv;
  do {
    HttpConnection::ReadIOBuffer* read_buf = connection->read_buf();
    // Increases read buffer size if necessary.
    if (read_buf->RemainingCapacity() == 0 && !read_buf->IncreaseCapacity()) {
      Close(connection->id());
      return;
    }

    rv = connection->socket()->Read(
        read_buf,
        read_buf->RemainingCapacity(),
        base::Bind(&SimpleHttpServer::OnReadCompleted,
                   weak_ptr_factory_.GetWeakPtr(),
                   connection->id()));
    if (rv == ERR_IO_PENDING)
      return;
    rv = HandleReadResult(connection, rv);
  } while (rv == OK);
}

void SimpleHttpServer::OnReadCompleted(int connection_id, int rv) {
  HttpConnection* connection = FindConnection(connection_id);
  if (!connection)  // It might be closed right before by write error.
    return;

  if (HandleReadResult(connection, rv) == OK)
    DoReadLoop(connection);
}

int SimpleHttpServer::HandleReadResult(HttpConnection* connection, int rv) {
  if (rv <= 0) {
    Close(connection->id());
    return rv == 0 ? ERR_CONNECTION_CLOSED : rv;
  }

  HttpConnection::ReadIOBuffer* read_buf = connection->read_buf();
  read_buf->DidRead(rv);

  while (read_buf->GetSize() > 0) {
    HttpServerRequestInfo* request = FindRequest(connection->id());
    if (!request) {
      std::unique_ptr<HttpServerRequestInfo> request_ptr(
          new HttpServerRequestInfo());
      size_t pos = 0;

      if (!ParseHeaders(read_buf->StartOfBuffer(), read_buf->GetSize(),
                        request_ptr.get(), &pos)) {
        // An error has occured. Close the connection.
        Close(connection->id());
        return ERR_CONNECTION_CLOSED;
      } else if (!pos) {
        // If pos is 0, all the data in read_buf has been consumed, but the
        // headers have not been fully parsed yet. Continue parsing when more data
        // rolls in.
        break;
      }

      if (request_ptr->HasHeaderValue("transfer-encoding", "chunked")) {
        std::unique_ptr<HttpChunkedDecoder> decoder(new HttpChunkedDecoder());
        id_to_decoder_[connection->id()] = std::move(decoder);
      }

      // consume header size
      read_buf->DidConsume(pos);

      // Sets peer address if exists.
      connection->socket()->GetPeerAddress(&request_ptr->peer);

      // assign request
      request = request_ptr.get();

      id_to_request_[connection->id()] = std::move(request_ptr);
    }

    const char kContentLength[] = "content-length";
    HttpChunkedDecoder* decoder = FindDecoder(connection->id());
    if (request->headers.count(kContentLength)) {
      size_t content_length = 0;
      const size_t kMaxBodySize = 100 << 20;
      if (!base::StringToSizeT(request->GetHeaderValue(kContentLength),
                               &content_length) ||
          content_length > kMaxBodySize) {
        SendResponse(connection->id(),
                     HttpServerResponseInfo::CreateFor500(
                         "request content-length too big or unknown: " +
                         request->GetHeaderValue(kContentLength)));
        Close(connection->id());
        return ERR_CONNECTION_CLOSED;
      }

      request->data.append(read_buf->StartOfBuffer(), read_buf->GetSize());
      read_buf->DidConsume(read_buf->GetSize());
      if (request->data.size() < content_length) {
        break;
      }

      // if read all content length
      delegate_->OnHttpRequest(connection->id(), *request);
    } else if (decoder) {
      int n = decoder->FilterBuf(read_buf->StartOfBuffer(),
                                 read_buf->GetSize());
      if (n > 0) {
        request->data.append(read_buf->StartOfBuffer(), n);
      }

      read_buf->DidConsume(read_buf->GetSize());

      if (decoder->reached_eof()) {
        delegate_->OnHttpRequest(connection->id(), *request);
      }
    } else {
      delegate_->OnHttpRequest(connection->id(), *request);
    }
  }


  if (HasClosedConnection(connection))
    return ERR_CONNECTION_CLOSED;

  return OK;
}

void SimpleHttpServer::DoWriteLoop(HttpConnection* connection) {
  int rv = OK;
  HttpConnection::QueuedWriteIOBuffer* write_buf = connection->write_buf();
  while (rv == OK && write_buf->GetSizeToWrite() > 0) {
    rv = connection->socket()->Write(
        write_buf,
        write_buf->GetSizeToWrite(),
        base::Bind(&SimpleHttpServer::OnWriteCompleted,
                   weak_ptr_factory_.GetWeakPtr(), connection->id()));
    if (rv == ERR_IO_PENDING || rv == OK)
      return;
    rv = HandleWriteResult(connection, rv);
  }
}

void SimpleHttpServer::OnWriteCompleted(int connection_id, int rv) {
  HttpConnection* connection = FindConnection(connection_id);
  if (!connection)  // It might be closed right before by read error.
    return;

  if (HandleWriteResult(connection, rv) == OK)
    DoWriteLoop(connection);
}

int SimpleHttpServer::HandleWriteResult(HttpConnection* connection, int rv) {
  if (rv < 0) {
    Close(connection->id());
    return rv;
  }

  connection->write_buf()->DidConsume(rv);
  return OK;
}

namespace {

//
// HTTP Request Parser
// This HTTP request parser uses a simple state machine to quickly parse
// through the headers.  The parser is not 100% complete, as it is designed
// for use in this simple test driver.
//
// Known issues:
//   - does not handle whitespace on first HTTP line correctly.  Expects
//     a single space between the method/url and url/protocol.

// Input character types.
enum header_parse_inputs {
  INPUT_LWS,
  INPUT_CR,
  INPUT_LF,
  INPUT_COLON,
  INPUT_DEFAULT,
  MAX_INPUTS,
};

// Parser states.
enum header_parse_states {
  ST_METHOD,     // Receiving the method
  ST_URL,        // Receiving the URL
  ST_PROTO,      // Receiving the protocol
  ST_HEADER,     // Starting a Request Header
  ST_NAME,       // Receiving a request header name
  ST_SEPARATOR,  // Receiving the separator between header name and value
  ST_VALUE,      // Receiving a request header value
  ST_DONE,       // Parsing is complete and successful
  ST_ERR,        // Parsing encountered invalid syntax.
  MAX_STATES
};

// State transition table
const int parser_state[MAX_STATES][MAX_INPUTS] = {
    /* METHOD    */ {ST_URL, ST_ERR, ST_ERR, ST_ERR, ST_METHOD},
    /* URL       */ {ST_PROTO, ST_ERR, ST_ERR, ST_URL, ST_URL},
    /* PROTOCOL  */ {ST_ERR, ST_HEADER, ST_NAME, ST_ERR, ST_PROTO},
    /* HEADER    */ {ST_ERR, ST_ERR, ST_NAME, ST_ERR, ST_ERR},
    /* NAME      */ {ST_SEPARATOR, ST_DONE, ST_ERR, ST_VALUE, ST_NAME},
    /* SEPARATOR */ {ST_SEPARATOR, ST_ERR, ST_ERR, ST_VALUE, ST_ERR},
    /* VALUE     */ {ST_VALUE, ST_HEADER, ST_NAME, ST_VALUE, ST_VALUE},
    /* DONE      */ {ST_DONE, ST_DONE, ST_DONE, ST_DONE, ST_DONE},
    /* ERR       */ {ST_ERR, ST_ERR, ST_ERR, ST_ERR, ST_ERR}};

// Convert an input character to the parser's input token.
int charToInput(char ch) {
  switch (ch) {
    case ' ':
    case '\t':
      return INPUT_LWS;
    case '\r':
      return INPUT_CR;
    case '\n':
      return INPUT_LF;
    case ':':
      return INPUT_COLON;
  }
  return INPUT_DEFAULT;
}

}  // namespace

bool SimpleHttpServer::ParseHeaders(const char* data,
                                    size_t data_len,
                                    HttpServerRequestInfo* info,
                                    size_t* ppos) {
  size_t& pos = *ppos;
  int state = ST_METHOD;
  std::string buffer;
  std::string header_name;
  std::string header_value;
  while (pos < data_len) {
    char ch = data[pos++];
    int input = charToInput(ch);
    int next_state = parser_state[state][input];

    bool transition = (next_state != state);
    HttpServerRequestInfo::HeadersMap::iterator it;
    if (transition) {
      // Do any actions based on state transitions.
      switch (state) {
        case ST_METHOD:
          info->method = buffer;
          buffer.clear();
          break;
        case ST_URL:
          info->path = buffer;
          buffer.clear();
          break;
        case ST_PROTO:
          if (buffer != "HTTP/1.1") {
            LOG(ERROR) << "Cannot handle request with protocol: " << buffer;
            next_state = ST_ERR;
          }
          buffer.clear();
          break;
        case ST_NAME:
          header_name = base::ToLowerASCII(buffer);
          buffer.clear();
          break;
        case ST_VALUE:
          base::TrimWhitespaceASCII(buffer, base::TRIM_LEADING, &header_value);
          it = info->headers.find(header_name);
          // See the second paragraph ("A sender MUST NOT generate multiple
          // header fields...") of tools.ietf.org/html/rfc7230#section-3.2.2.
          if (it == info->headers.end()) {
            info->headers[header_name] = header_value;
          } else {
            it->second.append(",");
            it->second.append(header_value);
          }
          buffer.clear();
          break;
        case ST_SEPARATOR:
          break;
      }
      state = next_state;
    } else {
      // Do any actions based on current state
      switch (state) {
        case ST_METHOD:
        case ST_URL:
        case ST_PROTO:
        case ST_VALUE:
        case ST_NAME:
          buffer.append(&ch, 1);
          break;
        case ST_DONE:
          // We got CR to get this far, also need the LF
          return (input == INPUT_LF);
        case ST_ERR:
          return false;
      }
    }
  }
  // No more characters, but we haven't finished parsing yet. Signal this to
  // the caller by setting |pos| to zero.
  pos = 0;
  return true;
}

HttpConnection* SimpleHttpServer::FindConnection(int connection_id) {
  auto it = id_to_connection_.find(connection_id);
  if (it == id_to_connection_.end())
    return nullptr;
  return it->second.get();
}

HttpServerRequestInfo* SimpleHttpServer::FindRequest(int connection_id) {
  auto it = id_to_request_.find(connection_id);
  if (it == id_to_request_.end()) {
    return nullptr;
  }
  return it->second.get();
}

HttpChunkedDecoder* SimpleHttpServer::FindDecoder(int connection_id) {
  auto it = id_to_decoder_.find(connection_id);
  if (it == id_to_decoder_.end()) {
    return nullptr;
  }
  return it->second.get();
}

// This is called after any delegate callbacks are called to check if Close()
// has been called during callback processing. Using the pointer of connection,
// |connection| is safe here because Close() deletes the connection in next run
// loop.
bool SimpleHttpServer::HasClosedConnection(HttpConnection* connection) {
  return FindConnection(connection->id()) != connection;
}

}  // namespace net

