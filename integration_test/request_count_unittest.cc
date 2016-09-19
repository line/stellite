//
// Copyright 2016 LINE Corporation
//

#include <memory>

#include "base/synchronization/waitable_event.h"
#include "net/base/net_errors.h"
#include "stellite/include/http_client.h"
#include "stellite/include/http_client_context.h"
#include "stellite/include/http_request.h"
#include "stellite/include/http_response.h"
#include "stellite/test/http_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace stellite {

class RequestCountCallback : public HttpResponseDelegate {
 public:
  RequestCountCallback()
      : on_test_completed_(true, false),
        request_count_(),
        response_count_(0) {
  }

  void OnHttpResponse(int request_id, const HttpResponse& response,
                      const char* data, size_t len) override {
    ++response_count_;
    if (response_count_ == request_count_) {
      on_test_completed_.Signal();
    }
  }

  void OnHttpStream(int request_id, const HttpResponse& response,
                    const char* stream_data, size_t len) override {
  }

  void OnHttpError(int request_id, int error_code,
                   const std::string& message) override {
    ++response_count_;
    if (response_count_ == request_count_) {
      on_test_completed_.Signal();
    }
  }

  void WaitForTest() {
    on_test_completed_.Wait();
  }

  void set_request_count(int request_count) {
    request_count_ = request_count;
  }

  base::WaitableEvent on_test_completed_;
  int request_count_;
  int response_count_;
};

class RequestCountTest : public testing::Test {
 public:
  RequestCountTest() {}

  void SetUp() override {
    HttpTestServer::Params server_params;
    server_params.server_type = HttpTestServer::TYPE_HTTP;
    server_params.port = 18000;
    http_server_.reset(new HttpTestServer(server_params));
    EXPECT_TRUE(http_server_->Start());

    HttpClientContext::Params client_params;
    client_context_.reset(new HttpClientContext(client_params));
    EXPECT_TRUE(client_context_->Initialize());
  }

  void TearDown() override {
    EXPECT_TRUE(client_context_->TearDown());
    http_server_->Shutdown();
  }

  ~RequestCountTest() override {}
  std::unique_ptr<HttpTestServer> http_server_;
  std::unique_ptr<HttpClientContext> client_context_;
};

TEST_F(RequestCountTest, CheckEitherResponseCountAreCorrectOrNot) {
  HttpRequest request;
  request.url = "http://localhost:18000/";
  request.method = "GET";

  // Try request 100 times
  std::unique_ptr<RequestCountCallback> response_callback(
      new RequestCountCallback());

  response_callback->set_request_count(100);
  HttpClient* client(
      client_context_->CreateHttpClient(response_callback.get()));
  for (int i = 0; i < 100; ++i) {
    client->Request(request);
  }

  response_callback->WaitForTest();
  client_context_->ReleaseHttpClient(client);
}


TEST_F(RequestCountTest, CheckRequestInvalidServerPort) {
  HttpRequest request;
  request.url = "http://localhost:9000/";
  request.method = "GET";

  // Try request 100 times
  std::unique_ptr<RequestCountCallback> response_callback(
      new RequestCountCallback());

  response_callback->set_request_count(100);
  HttpClient* client(client_context_->CreateHttpClient(
          response_callback.get()));
  for (int i = 0; i < 100; ++i) {
    client->Request(request);
  }

  response_callback->WaitForTest();
  client_context_->ReleaseHttpClient(client);
}

} // Stellite