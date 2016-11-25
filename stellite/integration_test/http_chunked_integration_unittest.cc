//
// Copyright 2016 LINE Corporation
//

#include <memory>

#include "base/synchronization/waitable_event.h"
#include "stellite/include/http_client.h"
#include "stellite/include/http_client_context.h"
#include "stellite/include/http_request.h"
#include "stellite/include/http_response.h"
#include "stellite/test/http_test_server.h"
#include "net/base/net_errors.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace stellite {

class HttpChunkedResponseCallback : public HttpResponseDelegate {
 public:
  HttpChunkedResponseCallback()
      : on_request_completed_(true, false),
        current_chunked_count_(0),
        request_completed_(false) {
  }

  void OnHttpResponse(int request_id, const HttpResponse& response,
                      const char* data, size_t len) override {
    on_request_completed_.Signal();
  }

  void OnHttpStream(int request_id, const HttpResponse& response,
                    const char* stream_data, size_t len) override {
    ++current_chunked_count_;

    if (len == 0) {
      request_completed_ = true;
      on_request_completed_.Signal();
    }
  }

  void OnHttpError(int request_id, int error_code,
                   const std::string& message) override {
    on_request_completed_.Signal();
  }

  void WaitForResponse() {
    on_request_completed_.Wait();
  }

  base::WaitableEvent on_request_completed_;
  int current_chunked_count_;
  bool request_completed_;
};

class HttpClientChunkedTest : public testing::Test {};

TEST_F(HttpClientChunkedTest, HttpChunkedResponse) {
  // Get 100 chunked data

  HttpTestServer::Params server_params;
  server_params.server_type = HttpTestServer::TYPE_HTTP;
  server_params.port = 18000;

  std::unique_ptr<HttpTestServer> server(new HttpTestServer(server_params));
  EXPECT_TRUE(server->Start());

  HttpClientContext::Params client_params;
  std::unique_ptr<HttpClientContext>
      client_context(new HttpClientContext(client_params));
  EXPECT_TRUE(client_context->Initialize());

  HttpRequest request;
  request.url = "http://localhost:18000/chunked";
  request.method = "GET";

  std::unique_ptr<HttpChunkedResponseCallback> response_callback(
      new HttpChunkedResponseCallback());
  HttpClient* client(client_context->CreateHttpClient(response_callback.get()));
  client->Stream(request);

  response_callback->WaitForResponse();

  EXPECT_GE(response_callback->current_chunked_count_, 100);
  EXPECT_TRUE(response_callback->request_completed_);

  client_context->ReleaseHttpClient(client);
  EXPECT_TRUE(client_context->TearDown());
  server->Shutdown();
}

} // namespace stellite