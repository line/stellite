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
#include "stellite/test/quic_mock_server.h"
#include "net/base/ip_endpoint.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace stellite {

class QuicClientResponseCallback : public HttpResponseDelegate {
 public:
  QuicClientResponseCallback()
      : on_request_complete_(true, false),
        is_response_received_(false) {
  }

  void OnHttpResponse(int request_id, const HttpResponse& response,
                      const char* data, size_t len) override {
    is_response_received_ = true;
    response_data_ = std::string(data, len);
    on_request_complete_.Signal();
  }

  void OnHttpStream(int request_id, const HttpResponse& response,
                    const char* stream_data, size_t len) override {
    on_request_complete_.Signal();
  }

  void OnHttpError(int request_id, int error_code,
                   const std::string& error_message) override {
    LOG(ERROR) << "QUIC client end-to-end error: " << error_message;
    on_request_complete_.Signal();
  }

  void WaitForResponse() {
    on_request_complete_.Wait();
  }

  const std::string& response_data() {
    return response_data_;
  }

  base::WaitableEvent on_request_complete_;
  std::string response_data_;
  bool is_response_received_;
};

class QuicClientEndToEndTest : public testing::Test {
 public:
  QuicClientEndToEndTest() {
  }

  void SetUp() override {
    // Initialize server thread
    quic_server_.reset(new QuicMockServer());
    quic_server_->SetCertificate("example.com.cert.pkcs8",
                                 "example.com.cert.pem");
    quic_server_->SetProxyPass("http://example.com:8080");
    quic_server_->Start(4430);

    // Initialize HTTP server
    HttpTestServer::Params http_server_params;
    http_server_params.server_type = HttpTestServer::TYPE_HTTP;
    http_server_params.port = 8080;
    http_server_.reset(new HttpTestServer(http_server_params));
    EXPECT_TRUE(http_server_->Start());

    // Initialize client
    HttpClientContext::Params client_context_params;
    client_context_params.using_quic = true;
    client_context_params.origin_to_force_quic_on = "example.com:4430";

    http_client_context_.reset(new HttpClientContext(client_context_params));
    EXPECT_TRUE(http_client_context_->Initialize());

    response_callback_.reset(new QuicClientResponseCallback());
    http_client_ = http_client_context_->CreateHttpClient(
        response_callback_.get());
  }

  void TearDown() override {
    http_client_context_->ReleaseHttpClient(http_client_);

    http_server_->Shutdown();
    quic_server_->Shutdown();
  }

  std::unique_ptr<QuicMockServer> quic_server_;
  std::unique_ptr<HttpTestServer> http_server_;
  std::unique_ptr<HttpClientContext> http_client_context_;
  std::unique_ptr<QuicClientResponseCallback> response_callback_;

  HttpClient* http_client_;
};

TEST_F(QuicClientEndToEndTest, QuicHttpGet) {
  HttpRequest request;
  request.method = "GET";
  request.url = "https://example.com:4430";
  http_client_->Request(request);

  response_callback_->WaitForResponse();
  EXPECT_TRUE(response_callback_->is_response_received_);
  EXPECT_EQ(response_callback_->response_data(), "get");
}

TEST_F(QuicClientEndToEndTest, QuicHttpPost) {
  HttpRequest request;
  request.method = "POST";
  request.url = "https://example.com:4430";

  std::string message = "hello world";
  request.upload_stream.write(message.c_str(), message.size());
  http_client_->Request(request);

  response_callback_->WaitForResponse();
  EXPECT_TRUE(response_callback_->is_response_received_);
  EXPECT_EQ(response_callback_->response_data(), "post");
}

} // namespace stellite