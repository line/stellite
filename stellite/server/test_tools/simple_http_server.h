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

#ifndef STELLITE_SERVER_TEST_TOOLS_SIMPLE_HTTP_SERVER_H_
#define STELLITE_SERVER_TEST_TOOLS_SIMPLE_HTTP_SERVER_H_

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "net/http/http_status_code.h"

namespace net {

class HttpChunkedDecoder;
class HttpConnection;
class HttpServerRequestInfo;
class HttpServerResponseInfo;
class IPEndPoint;
class ServerSocket;
class StreamSocket;

class SimpleHttpServer {
 public:
  // Delegate to handle http/websocket events. Beware that it is not safe to
  // destroy the HttpServer in any of these callbacks.
  class Delegate {
   public:
    virtual ~Delegate() {}

    virtual void OnConnect(int connection_id) = 0;
    virtual void OnClose(int connection_id) = 0;
    virtual void OnHttpRequest(int connection_id,
                               const HttpServerRequestInfo& info) = 0;
  };

  // Instantiates a http server with |server_socket| which already started
  // listening, but not accepting.  This constructor schedules accepting
  // connections asynchronously in case when |delegate| is not ready to get
  // callbacks yet.
  SimpleHttpServer(std::unique_ptr<ServerSocket> server_socket,
                   SimpleHttpServer::Delegate* delegate);
  ~SimpleHttpServer();

  void SendRaw(int connection_id, const std::string& data);
  // TODO(byungchul): Consider replacing function name with SendResponseInfo
  void SendResponse(int connection_id, const HttpServerResponseInfo& response);
  void Send(int connection_id,
            HttpStatusCode status_code,
            const std::string& data,
            const std::string& mime_type);
  void Send200(int connection_id,
               const std::string& data,
               const std::string& mime_type);
  void Send404(int connection_id);
  void Send500(int connection_id, const std::string& message);

  void Close(int connection_id);
  void CloseConnection(int connection_id);
  void CloseRequest(int connection_id);
  void CloseDecoder(int connection_id);

  void SetReceiveBufferSize(int connection_id, int32_t size);
  void SetSendBufferSize(int connection_id, int32_t size);

  // Copies the local address to |address|. Returns a network error code.
  int GetLocalAddress(IPEndPoint* address);

 private:
  friend class HttpServerTest;

  void DoAcceptLoop();
  void OnAcceptCompleted(int rv);
  int HandleAcceptResult(int rv);

  void DoReadLoop(HttpConnection* connection);
  void OnReadCompleted(int connection_id, int rv);
  int HandleReadResult(HttpConnection* connection, int rv);

  void DoWriteLoop(HttpConnection* connection);
  void OnWriteCompleted(int connection_id, int rv);
  int HandleWriteResult(HttpConnection* connection, int rv);

  // Expects the raw data to be stored in recv_data_. If parsing is successful,
  // will remove the data parsed from recv_data_, leaving only the unused
  // recv data. If all data has been consumed successfully, but the headers are
  // not fully parsed, *pos will be set to zero. Returns false if an error is
  // encountered while parsing, true otherwise.
  bool ParseHeaders(const char* data,
                    size_t data_len,
                    HttpServerRequestInfo* info,
                    size_t* pos);

  HttpConnection* FindConnection(int connection_id);
  HttpServerRequestInfo* FindRequest(int connection_id);
  HttpChunkedDecoder* FindDecoder(int connection_id);

  // Whether or not Close() has been called during delegate callback processing.
  bool HasClosedConnection(HttpConnection* connection);

  const std::unique_ptr<ServerSocket> server_socket_;
  std::unique_ptr<StreamSocket> accepted_socket_;
  SimpleHttpServer::Delegate* const delegate_;

  int last_id_;
  std::map<int, std::unique_ptr<HttpConnection>> id_to_connection_;
  std::map<int, std::unique_ptr<HttpServerRequestInfo>> id_to_request_;
  std::map<int, std::unique_ptr<HttpChunkedDecoder>> id_to_decoder_;

  base::WeakPtrFactory<SimpleHttpServer> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SimpleHttpServer);
};

}  // namespace net

#endif  // STELLITE_SERVER_TEST_TOOLS_SIMPLE_HTTP_SERVER_H_
