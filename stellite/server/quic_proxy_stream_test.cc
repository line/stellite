// Copyright 2016 LINE Corporation
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

#include <memory>

#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "net/http/http_response_headers.h"
#include "net/quic/core/spdy_utils.h"
#include "net/quic/test_tools/crypto_test_utils.h"
#include "net/server/http_server_request_info.h"
#include "net/server/http_server_response_info.h"
#include "net/socket/server_socket.h"
#include "net/socket/tcp_server_socket.h"
#include "net/tools/quic/test_tools/mock_quic_server_session_visitor.h"
#include "stellite/fetcher/http_fetcher.h"
#include "stellite/fetcher/http_request_context_getter.h"
#include "stellite/server/quic_proxy_session.h"
#include "stellite/server/quic_proxy_stream.h"
#include "stellite/server/test_tools/simple_http_server.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::StrictMock;
using base::StringPiece;
using net::test::MockQuicConnection;
using net::test::MockQuicConnectionHelper;

namespace net {
namespace test {

namespace {

class QuicProxyStreamChild : public QuicProxyStream {
 public:
  QuicProxyStreamChild(QuicStreamId id, QuicSpdySession* session,
                       stellite::HttpFetcher* fetcher, GURL proxy_pass,
                       base::RunLoop* run_loop)
      : QuicProxyStream(id, session, fetcher, proxy_pass),
        run_loop_(run_loop),
        error_code_(0),
        is_call_task_complete_(false),
        is_call_task_header_(false),
        is_call_task_stream_(false),
        is_call_task_error_(false),
        is_call_send_error_res_(false) {
  }

  ~QuicProxyStreamChild() override {}

  void OnTaskComplete(int request_id, const URLFetcher* source,
                      const HttpResponseInfo* response_info) override {
    is_call_task_complete_ = true;
    response_info_ = *response_info;
    run_loop_->Quit();
  }

  void OnTaskHeader(int request_id, const URLFetcher* source,
                    const HttpResponseInfo* response_info) override {
    response_info_ = *response_info;
    is_call_task_header_ = true;
  }

  void OnTaskStream(int request_id, const char* data, size_t len,
                    bool fin) override {
    is_call_task_stream_ = true;
    response_body_.append(std::string(data, len));
    if (fin) {
      run_loop_->Quit();
    }
  }

  void OnTaskError(int request_id, const URLFetcher* source,
                   int error_code) override {
    is_call_task_error_ = true;
    run_loop_->Quit();
  }

  void SendErrorResponse(int error_code, const std::string& message) override {
    error_code_ = error_code;
    error_message_ = message;

    is_call_send_error_res_ = true;
    run_loop_->Quit();
  }

  HttpResponseInfo* response_info() { return &response_info_; }

  std::string response_body() { return response_body_; }

  int error_code() { return error_code_; }
  std::string error_message() { return error_message_; }

  bool is_call_task_complet() { return is_call_task_complete_; }
  bool is_call_task_header() { return is_call_task_header_; }
  bool is_call_task_stream() { return is_call_task_stream_; }
  bool is_call_task_error() { return is_call_task_error_; }
  bool is_call_send_error_res() { return is_call_send_error_res_; }

 private:
  base::RunLoop* run_loop_;

  HttpResponseInfo response_info_;
  std::string response_body_;

  int error_code_;
  std::string error_message_;

  bool is_call_task_complete_;
  bool is_call_task_header_;
  bool is_call_task_stream_;
  bool is_call_task_error_;
  bool is_call_send_error_res_;

  DISALLOW_COPY_AND_ASSIGN(QuicProxyStreamChild);
};

class QuicProxyStreamTest : public ::testing::Test,
                            public SimpleHttpServer::Delegate {
 public:
  QuicProxyStreamTest()
      : connection_(
          new StrictMock<MockQuicConnection>(&helper_,
                                             &alarm_factory_,
                                             Perspective::IS_SERVER,
                                             AllSupportedVersions())),
        session_(connection_),
        upload_offset_(0),
        chunked_response_(false) {}

  ~QuicProxyStreamTest() override {}

  void SetUp() override {
    // init fetcher
    context_getter_ =
        new stellite::HttpRequestContextGetter(
            stellite::HttpRequestContextGetter::Params(),
            base::ThreadTaskRunnerHandle::Get());

    fetcher_.reset(new stellite::HttpFetcher(context_getter_));

    // init backend server
    std::unique_ptr<ServerSocket> server_socket(
        new TCPServerSocket(nullptr, NetLogSource()));
    server_socket->ListenWithAddressAndPort("127.0.0.1", 0, 1);
    http_server_.reset(new SimpleHttpServer(std::move(server_socket), this));
    http_server_->GetLocalAddress(&server_address_);

    // setup session
    session_.config()->SetInitialStreamFlowControlWindowToSend(
        kInitialStreamFlowControlWindowForTest);
    session_.config()->SetInitialSessionFlowControlWindowToSend(
        kInitialSessionFlowControlWindowForTest);

    // init stream
    std::string proxy_pass = base::StringPrintf("http://127.0.0.1:%d/",
                                                server_address_.port());

    stream_ = new QuicProxyStreamChild(::net::test::kClientDataStreamId1,
                                       &session_, fetcher_.get(),
                                       GURL(proxy_pass), &run_loop_);

    // Register stream_ in dynamic_stream_map_ and pass ownership to session_.
    session_.ActivateStream(base::WrapUnique(stream_));
  }

  // implements HttpServer::Delegate
  void OnConnect(int connection_id) override {}
  void OnClose(int connection_id) override {}

  void HandleRedirect(int connection_id, const HttpServerRequestInfo& info) {
    HttpServerResponseInfo response(HTTP_FOUND);
    response.AddHeader("Location", "http://www.example.com/get");
    http_server_->SendResponse(connection_id, response);
  }

  void OnHttpRequest(int connection_id,
                     const HttpServerRequestInfo& request) override {
    backend_request_info_ = request;
    backend_request_connectoin_id_ = connection_id;

    if (request.path == "/redirect") {
      HandleRedirect(connection_id, request);
      return;
    }

    SendResponse(connection_id, request.data, "plain/text");
  }

  std::string IntToHex(uint32_t size) {
    const char hex_map[] = "0123456789abcdef";
    std::stringstream res;
    while (size != 0) {
      res.write(&hex_map[size % arraysize(hex_map)], 1);
      size = size / arraysize(hex_map);
    }
    std::string hex(res.str());
    std::reverse(hex.begin(), hex.end());
    return hex;
  }

  void SendResponse(int connection_id,
                    const std::string& data,
                    const std::string& mime_type) {
    HttpServerResponseInfo response(HTTP_OK);

    if (chunked_response_) {
      response.AddHeader(HttpRequestHeaders::kTransferEncoding, "chunked");
    } else {
      response.SetContentHeaders(data.length(), data);
    }

    // send header
    http_server_->SendRaw(connection_id, response.Serialize());

    // send payload
    if (chunked_response_) {
      std::string chunked_upload_payload;
      chunked_upload_payload.append(IntToHex(data.size()));
      chunked_upload_payload.append("\r\n");
      chunked_upload_payload.append(data);
      chunked_upload_payload.append("\r\n");
      chunked_upload_payload.append("0\r\n\r\n");
      http_server_->SendRaw(connection_id, chunked_upload_payload);
    } else {
      http_server_->SendRaw(connection_id, data);
    }
  }

  QuicProxyStreamChild* stream() { return stream_; }

  void PushRequestHeader(const SpdyHeaderBlock& request_headers, bool fin) {
    std::string headers_string =
        net::SpdyUtils::SerializeUncompressedHeaders(request_headers);
    stream_->OnStreamHeaders(headers_string);
    stream_->OnStreamHeadersComplete(fin, headers_string.size());
  }

  void PushRequestBody(const std::string& body, bool fin) {
    stream_->OnStreamFrame(
        QuicStreamFrame(stream_->id(), fin, /*offset=*/upload_offset_, body));
    upload_offset_ += body.size();
  }

  std::string BackendDebugString() {
    HttpServerRequestInfo::HeadersMap& headers = backend_request_info_.headers;
    std::string res;
    for (HttpServerRequestInfo::HeadersMap::iterator it = headers.begin();
         it != headers.end(); ++it) {
      res.append(it->first);
      res.append(": ");
      res.append(it->second);
      res.append("\n");
    }
    return res;
  }

  void set_chunked_response(bool chunked_response) {
    chunked_response_ = chunked_response;
  }

  // http_fether's context getter
  scoped_refptr<stellite::HttpRequestContextGetter> context_getter_;
  std::unique_ptr<stellite::HttpFetcher> fetcher_;

  // setup backend http server
  std::unique_ptr<SimpleHttpServer> http_server_;
  IPEndPoint server_address_;

  // setup stream
  MockQuicConnectionHelper helper_;
  MockAlarmFactory alarm_factory_;
  StrictMock<MockQuicConnection>* connection_;
  StrictMock<MockQuicSpdySession> session_;

  QuicProxyStreamChild* stream_;

  uint64_t upload_offset_;

  bool chunked_response_;

  // backend request info
  int backend_request_connectoin_id_;
  net::HttpServerRequestInfo backend_request_info_;

  base::RunLoop run_loop_;
};

}  // anonymous namespace

TEST_F(QuicProxyStreamTest, SendProxyRequestToBackendWithGetMethod) {
  SpdyHeaderBlock request_headers;
  request_headers[":host"] = "";
  request_headers[":authority"] = "www.example.com";
  request_headers[":path"] = "/get";
  request_headers[":method"] = "GET";
  request_headers[":version"] = "HTTP/1.1";

  PushRequestHeader(request_headers, true);

  run_loop_.Run();

  ASSERT_EQ(backend_request_info_.method, "GET");
  ASSERT_EQ(backend_request_info_.path, "/get");
}

TEST_F(QuicProxyStreamTest, SendProxyGetWithPayload) {
  SpdyHeaderBlock request_headers;
  request_headers[":host"] = "";
  request_headers[":authority"] = "www.example.com";
  request_headers[":path"] = "/get";
  request_headers[":method"] = "GET";
  request_headers[":version"] = "HTTP/1.1";
  request_headers["content-length"] = "11";

  PushRequestHeader(request_headers, false);

  std::string data = "hello world";
  PushRequestBody(data, true);

  run_loop_.Run();

  ASSERT_TRUE(stream()->is_call_send_error_res());
  ASSERT_EQ(stream()->error_code(), 400);
}

TEST_F(QuicProxyStreamTest, SendProxyRequestToBackendWithPostMethod) {
  SpdyHeaderBlock request_headers;
  request_headers[":host"] = "";
  request_headers[":authority"] = "www.example.com";
  request_headers[":path"] = "/post";
  request_headers[":method"] = "POST";
  request_headers[":version"] = "HTTP/1.1";
  request_headers["content-length"] = "11";

  PushRequestHeader(request_headers, false);

  std::string data = "hello world";
  PushRequestBody(data, true);

  run_loop_.Run();

  ASSERT_EQ(backend_request_info_.method, "POST");
  ASSERT_EQ(backend_request_info_.path, "/post");
  ASSERT_EQ(backend_request_info_.data, data);
}

TEST_F(QuicProxyStreamTest, SendProxyRequestToBackendWithPutMethod) {
  SpdyHeaderBlock request_headers;
  request_headers[":host"] = "";
  request_headers[":authority"] = "www.example.com";
  request_headers[":path"] = "/put";
  request_headers[":method"] = "PUT";
  request_headers[":version"] = "HTTP/1.1";
  request_headers["content-length"] = "11";

  PushRequestHeader(request_headers, false);

  std::string data = "hello world";
  PushRequestBody(data, true);

  run_loop_.Run();

  ASSERT_EQ(backend_request_info_.method, "PUT");
  ASSERT_EQ(backend_request_info_.path, "/put");
  ASSERT_EQ(backend_request_info_.data, data);
}

TEST_F(QuicProxyStreamTest, SendProxyRequestToBackendWithDeleteMethod) {
  SpdyHeaderBlock request_headers;
  request_headers[":host"] = "";
  request_headers[":authority"] = "www.example.com";
  request_headers[":path"] = "/delete";
  request_headers[":method"] = "DELETE";
  request_headers[":version"] = "HTTP/1.1";

  PushRequestHeader(request_headers, true);

  run_loop_.Run();

  ASSERT_EQ(backend_request_info_.method, "DELETE");
  ASSERT_EQ(backend_request_info_.path, "/delete");
}

TEST_F(QuicProxyStreamTest, SendProxyRequestToBackendWithHeadMethod) {
  SpdyHeaderBlock request_headers;
  request_headers[":host"] = "";
  request_headers[":authority"] = "www.example.com";
  request_headers[":path"] = "/head";
  request_headers[":method"] = "HEAD";
  request_headers[":version"] = "HTTP/1.1";

  PushRequestHeader(request_headers, true);

  run_loop_.Run();

  ASSERT_EQ(backend_request_info_.method, "HEAD");
  ASSERT_EQ(backend_request_info_.path, "/head");
}

TEST_F(QuicProxyStreamTest, SendProxyRequestToBackendWithPatchMethod) {
  SpdyHeaderBlock request_headers;
  request_headers[":host"] = "";
  request_headers[":authority"] = "www.example.com";
  request_headers[":path"] = "/patch";
  request_headers[":method"] = "PATCH";
  request_headers[":version"] = "HTTP/1.1";
  request_headers["content-length"] = "11";

  PushRequestHeader(request_headers, false);

  std::string data = "hello world";
  PushRequestBody(data, true);

  run_loop_.Run();

  ASSERT_EQ(backend_request_info_.method, "PATCH");
  ASSERT_EQ(backend_request_info_.path, "/patch");
  ASSERT_EQ(backend_request_info_.data, data);
}

TEST_F(QuicProxyStreamTest, SendPostRequestButPayloadIsZero) {
  SpdyHeaderBlock request_headers;
  request_headers[":host"] = "";
  request_headers[":authority"] = "www.example.com";
  request_headers[":path"] = "/post";
  request_headers[":method"] = "POST";
  request_headers[":version"] = "HTTP/1.1";

  PushRequestHeader(request_headers, true);

  run_loop_.Run();

  ASSERT_TRUE(stream()->is_call_send_error_res());
}

TEST_F(QuicProxyStreamTest, SendPutRequestButPayloadIsZero) {
  SpdyHeaderBlock request_headers;
  request_headers[":host"] = "";
  request_headers[":authority"] = "www.example.com";
  request_headers[":path"] = "/put";
  request_headers[":method"] = "PUT";
  request_headers[":version"] = "HTTP/1.1";

  PushRequestHeader(request_headers, true);

  run_loop_.Run();

  ASSERT_TRUE(stream()->is_call_send_error_res());
}

TEST_F(QuicProxyStreamTest, SendPatchRequestButPayloadIsZero) {
  SpdyHeaderBlock request_headers;
  request_headers[":host"] = "";
  request_headers[":authority"] = "www.example.com";
  request_headers[":path"] = "/patch";
  request_headers[":method"] = "PATCH";
  request_headers[":version"] = "HTTP/1.1";

  PushRequestHeader(request_headers, true);

  run_loop_.Run();

  ASSERT_TRUE(stream()->is_call_send_error_res());
}

TEST_F(QuicProxyStreamTest, CheckHeaderXForwardedFor) {
  SpdyHeaderBlock request_headers;
  request_headers[":host"] = "";
  request_headers[":authority"] = "www.example.com";
  request_headers[":path"] = "/";
  request_headers[":method"] = "GET";
  request_headers[":version"] = "HTTP/1.1";

  PushRequestHeader(request_headers, true);

  run_loop_.Run();

  ASSERT_EQ(backend_request_info_.method, "GET");

  std::string xff = backend_request_info_.GetHeaderValue("x-forwarded-for");
  ASSERT_TRUE(bool(xff.size()));
  ASSERT_EQ(xff, "127.0.0.1");
}

TEST_F(QuicProxyStreamTest, CheckProxyXForwardedFor) {
  SpdyHeaderBlock request_headers;
  request_headers[":host"] = "";
  request_headers[":authority"] = "www.example.com";
  request_headers[":path"] = "/";
  request_headers[":method"] = "GET";
  request_headers[":version"] = "HTTP/1.1";

  std::string client_host = "1.2.3.4";
  request_headers["x-forwarded-for"] = client_host;

  PushRequestHeader(request_headers, true);

  run_loop_.Run();

  ASSERT_EQ(backend_request_info_.method, "GET");

  std::string xff = backend_request_info_.GetHeaderValue("x-forwarded-for");
  ASSERT_TRUE(bool(xff.size()));
  ASSERT_EQ(xff.substr(0, client_host.size()), client_host);
}

TEST_F(QuicProxyStreamTest, CheckProxyXForwardedHost) {
  SpdyHeaderBlock request_headers;
  request_headers[":host"] = "";
  request_headers[":authority"] = "www.example.com";
  request_headers[":path"] = "/";
  request_headers[":method"] = "GET";
  request_headers[":version"] = "HTTP/1.1";

  std::string host = "line.me";
  request_headers["host"] = host;

  PushRequestHeader(request_headers, true);

  run_loop_.Run();

  ASSERT_EQ(backend_request_info_.method, "GET");

  std::string xfh = backend_request_info_.GetHeaderValue("x-forwarded-host");
  ASSERT_EQ(xfh, host);
}

TEST_F(QuicProxyStreamTest, CheckChunkToUploadRequest) {
  SpdyHeaderBlock request_headers;
  request_headers[":host"] = "";
  request_headers[":authority"] = "www.example.com";
  request_headers[":path"] = "/chunked";
  request_headers[":method"] = "POST";
  request_headers[":version"] = "HTTP/1.1";
  request_headers["transfer-encoding"] = "chunked";

  PushRequestHeader(request_headers, false);

  PushRequestBody("hello", false);
  PushRequestBody("world", true);

  run_loop_.Run();

  ASSERT_EQ("helloworld", backend_request_info_.data);
}

TEST_F(QuicProxyStreamTest, CheckProxyResponse200) {
  SpdyHeaderBlock request_headers;
  request_headers[":host"] = "";
  request_headers[":authority"] = "www.example.com";
  request_headers[":path"] = "/get";
  request_headers[":method"] = "GET";
  request_headers[":version"] = "HTTP/1.1";

  PushRequestHeader(request_headers, true);

  run_loop_.Run();

  ASSERT_EQ(backend_request_info_.method, "GET");
  ASSERT_TRUE(stream()->is_call_task_header());
  ASSERT_EQ(stream()->response_info()->headers->response_code(), 200);
}

TEST_F(QuicProxyStreamTest, CheckChunkedResponse) {
  // set chunked_response from backend_server to quic_proxy_stream
  set_chunked_response(true);

  SpdyHeaderBlock request_headers;
  request_headers[":host"] = "";
  request_headers[":authority"] = "www.example.com";
  request_headers[":path"] = "/post";
  request_headers[":method"] = "POST";
  request_headers[":version"] = "HTTP/1.1";
  request_headers["content-length"] = "11";

  PushRequestHeader(request_headers, false);

  std::string body = "hello world";
  PushRequestBody(body, true);

  run_loop_.Run();

  HttpResponseHeaders* headers = stream()->response_info()->headers.get();
  ASSERT_TRUE(headers->HasHeaderValue("transfer-encoding", "chunked"));
}

TEST_F(QuicProxyStreamTest, CheckStopRedirectWhen3xxReceived) {
  SpdyHeaderBlock request_headers;
  request_headers[":host"] = "";
  request_headers[":authority"] = "www.example.com";
  request_headers[":path"] = "/redirect";
  request_headers[":method"] = "GET";
  request_headers[":version"] = "HTTP/1.1";

  PushRequestHeader(request_headers, true);

  run_loop_.Run();

  HttpResponseHeaders* headers = stream()->response_info()->headers.get();
  ASSERT_TRUE(
      HttpResponseHeaders::IsRedirectResponseCode(headers->response_code()));
}

}  // namespace test
}  // namespace net
