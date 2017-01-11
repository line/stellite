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

#include "stellite/include/http_client.h"

#include <memory>

#include "base/run_loop.h"
#include "base/time/time.h"
#include "stellite/include/http_client_context.h"
#include "stellite/include/http_request.h"
#include "stellite/include/http_response.h"
#include "stellite/test/http_test_server.h"
#include "net/base/net_errors.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "net/server/http_server.h"

namespace stellite {

class HttpClientResponseCallback : public HttpResponseDelegate {
 public:
  HttpClientResponseCallback()
      : request_completed_(false),
        last_error_code_(0) {
    start_time_ = base::TimeTicks::Now();
  }

  void OnHttpResponse(int request_id, const HttpResponse& response,
                      const char* data, size_t len) override {
    request_completed_ = true;
    response_ = response;
    end_time_ = base::TimeTicks::Now();
    run_loop_.Quit();
  }

  void OnHttpStream(int request_id, const HttpResponse& response,
                    const char* stream_data, size_t len,
                    bool is_last) override {
  }

  void OnHttpError(int request_id, int error_code,
                   const std::string& message) override {
    last_error_code_ = error_code;
    end_time_ = base::TimeTicks::Now();
    run_loop_.Quit();
  }

  void WaitForResponse() {
    run_loop_.Run();
  }

  bool request_complete() {
    return request_completed_;
  }

  int64_t request_time() {
    return (end_time_ - start_time_).InMilliseconds();
  }

  int last_error() {
    return last_error_code_;
  }

  base::RunLoop run_loop_;

  bool request_completed_;
  HttpResponse response_;

  base::TimeTicks start_time_;
  base::TimeTicks end_time_;

  int last_error_code_;
};

class HttpClientTest : public testing::Test {
 public:
  HttpClientTest() {}

  void SetUp() override {
    HttpTestServer::Params http_params;
    http_params.port = 8888;
    http_server_.reset(new HttpTestServer(http_params));
    http_server_->Start();

    HttpTestServer::Params https_params;
    https_params.port = 9999;
    https_params.server_type = HttpTestServer::TYPE_HTTPS;
    https_server_.reset(new HttpTestServer(https_params));
    https_server_->Start();

    HttpClientContext::Params params;
    params.using_quic = false;
    params.using_http2 = false;

    context_.reset(new HttpClientContext(params));
    context_->Initialize();

    response_callback_.reset(new HttpClientResponseCallback());
    http_client_ = context_->CreateHttpClient(response_callback_.get());
  }

  void TearDown() override {
    context_->ReleaseHttpClient(http_client_);
    http_client_ = nullptr;

    context_->TearDown();

    http_server_->Shutdown();
    https_server_->Shutdown();
  }

  std::unique_ptr<HttpTestServer> http_server_;
  std::unique_ptr<HttpTestServer> https_server_;
  std::unique_ptr<HttpClientContext> context_;
  std::unique_ptr<HttpClientResponseCallback> response_callback_;

  HttpClient* http_client_;
};

TEST_F(HttpClientTest, TestHttp11Get) {
  HttpRequest request;
  request.url = "http://example.com:8888";
  request.request_type = HttpRequest::GET;
  http_client_->Request(request);

  response_callback_->WaitForResponse();

  EXPECT_TRUE(response_callback_->request_complete());
}

TEST_F(HttpClientTest, TestHttp11Post) {
  HttpRequest request;
  request.url = "http://example.com:8888";
  request.request_type = HttpRequest::POST;
  http_client_->Request(request);

  response_callback_->WaitForResponse();
  EXPECT_TRUE(response_callback_->request_complete());
}

TEST_F(HttpClientTest, TestHttp11Put) {
  HttpRequest request;
  request.url = "http://example.com:8888";
  request.request_type = HttpRequest::PUT;
  http_client_->Request(request);

  response_callback_->WaitForResponse();
  EXPECT_TRUE(response_callback_->request_complete());
}

TEST_F(HttpClientTest, TestHttp11Delete) {
  HttpRequest request;
  request.url = "http://example.com:8888";
  request.request_type = HttpRequest::DELETE_REQUEST;
  http_client_->Request(request);

  response_callback_->WaitForResponse();
  EXPECT_TRUE(response_callback_->request_complete());
}

TEST_F(HttpClientTest, TestHttps) {
  HttpRequest request;
  request.url = "https://example.com:9999";
  request.request_type = HttpRequest::GET;
  http_client_->Request(request);

  response_callback_->WaitForResponse();
  EXPECT_TRUE(response_callback_->request_complete());
}

TEST_F(HttpClientTest, TestConnectionRefused) {
  HttpRequest request;

  // cannot receive an ACK
  request.url = "http://example.com:81";
  request.request_type = HttpRequest::GET;
  http_client_->Request(request, 100); // 100 ms

  response_callback_->WaitForResponse();
  EXPECT_EQ(response_callback_->last_error(), net::ERR_CONNECTION_REFUSED);
}

TEST_F(HttpClientTest, TestHttpRequestConnectionTimeout) {
  HttpRequest request;
  request.url = "http://example.com:8888/pending?delay=2000";
  request.request_type = HttpRequest::GET;
  http_client_->Request(request, 100); // 100 ms

  response_callback_->WaitForResponse();
  EXPECT_EQ(response_callback_->last_error(), net::ERR_CONNECTION_TIMED_OUT);
}

TEST_F(HttpClientTest, TestHttpRequestTimeout) {
  HttpRequest request;
  request.url = "http://example.com:8888/send_and_wait?delay=2000";
  request.request_type = HttpRequest::GET;
  http_client_->Request(request, 100); // 100 ms

  response_callback_->WaitForResponse();
  EXPECT_EQ(response_callback_->last_error(), net::ERR_TIMED_OUT);
}

TEST_F(HttpClientTest, ShutdownClientContextWhenRequesetArePending) {
  HttpRequest request;
  request.url = "http://example.com:81";
  request.request_type = HttpRequest::GET;
  http_client_->Request(request, 10000); // 10000 ms
}

TEST_F(HttpClientTest, GzipRequest) {
  HttpRequest request;
  request.url = "http://example.com:8888/gzip";
  request.request_type = HttpRequest::GET;

  // set deflate or gzip
  request.headers.SetHeader("Accept-Encoding", "gzip");
  http_client_->Request(request);

  response_callback_->WaitForResponse();

  std::string content_encoding;
  size_t iter = 0;
  const HttpResponseHeader& response_header =
      response_callback_->response_.headers;
  bool has_header = response_header.EnumerateHeader(&iter,
                                                    "Content-Encoding",
                                                    &content_encoding);
  EXPECT_TRUE(has_header);
  EXPECT_EQ(content_encoding, "gzip");
}

TEST_F(HttpClientTest, DeflateRequest) {
  HttpRequest request;
  request.url = "http://example.com:8888/deflate";
  request.request_type = HttpRequest::GET;

  // set deflate or gzip
  request.headers.SetHeader("Accept-Encoding", "deflate");
  http_client_->Request(request);

  response_callback_->WaitForResponse();

  std::string content_encoding;
  size_t iter = 0;
  const HttpResponseHeader& response_header =
      response_callback_->response_.headers;
  bool has_header = response_header.EnumerateHeader(&iter,
                                                   "Content-Encoding",
                                                    &content_encoding);
  EXPECT_TRUE(has_header);
  EXPECT_EQ(content_encoding, "deflate");
}

TEST_F(HttpClientTest, CancelHttpClientContextAfterRequest) {
  HttpRequest request;
  request.url = "https://example.com";
  request.request_type = HttpRequest::GET;
  http_client_->Request(request);

  context_->CancelAll();

  http_client_->Request(request);

  response_callback_->WaitForResponse();
  EXPECT_FALSE(response_callback_->request_complete());
}

} // namespace stellite
