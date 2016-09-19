//
// Copyright 2016 LINE Corporation
//

#include <memory>

#include "base/synchronization/waitable_event.h"
#include "net/base/ip_endpoint.h"
#include "stellite/include/http_client.h"
#include "stellite/include/http_client_context.h"
#include "stellite/include/http_request.h"
#include "stellite/include/http_response.h"
#include "stellite/test/quic_mock_server.h"
#include "stellite/test/http_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace stellite {

class QuicDiscoveryCallback : public HttpResponseDelegate {
 public:
  QuicDiscoveryCallback()
      : on_response_received_(true, false) {
  }

  void OnHttpResponse(int request_id, const HttpResponse& response,
                      const char* data, size_t len) override {
    response_ = response;
    on_response_received_.Signal();
    response_body_.append(data, len);
  }

  void OnHttpStream(int request_id, const HttpResponse& response,
                    const char* stream_data, size_t len) override {
  }

  void OnHttpError(int request_id, int error_code,
                   const std::string& error_message) override {
    on_response_received_.Signal();
  }

  void ResetSync() {
    on_response_received_.Reset();
  }

  void Wait() {
    on_response_received_.Wait();
  }

  base::WaitableEvent on_response_received_;
  HttpResponse response_;
  std::string response_body_;
};

class QuicDiscoveryTest : public testing::Test {
 public:
  QuicDiscoveryTest() {}

  void SetUp() override {
    // Initialize QUIC server
    quic_server_.reset(new QuicMockServer());
    quic_server_->SetProxyPass("http://example.com:4430");
    quic_server_->SetCertificate("example.com.cert.pkcs8",
                                 "example.com.cert.pem");
    quic_server_->Start(4430);

    // Initialize HTTP server
    HttpTestServer::Params http_server_params;
    http_server_params.server_type = HttpTestServer::TYPE_HTTPS;
    http_server_params.port = 4430;
    http_server_params.alternative_services =
        "quic=\"example.com:4430\"";
    http_server_.reset(new HttpTestServer(http_server_params));
    EXPECT_TRUE(http_server_->Start());

    // Initialize client
    HttpClientContext::Params client_context_params;
    client_context_params.using_quic = true;
    client_context_params.using_quic_disk_cache = true;

    http_client_context_.reset(new HttpClientContext(client_context_params));
    EXPECT_TRUE(http_client_context_->Initialize());

    response_callback_.reset(new QuicDiscoveryCallback());
    http_client_ = http_client_context_->CreateHttpClient(
        response_callback_.get());
  }

  void TearDown() override {
    http_client_context_->ReleaseHttpClient(http_client_);
    quic_server_->Shutdown();
    http_server_->Shutdown();
  }

  std::unique_ptr<QuicMockServer> quic_server_;
  std::unique_ptr<HttpTestServer> http_server_;
  std::unique_ptr<HttpClientContext> http_client_context_;
  std::unique_ptr<QuicDiscoveryCallback> response_callback_;

  HttpClient* http_client_;
};

TEST_F(QuicDiscoveryTest, QuicDiscoveryOnHttps) {
  for (int i = 0; i < 2; ++i) {
    HttpRequest request;
    request.method = "GET";
    request.url = "https://example.com:4430/pending?delay=50";
    http_client_->Request(request);

    response_callback_->ResetSync();
    response_callback_->Wait();

    EXPECT_EQ(response_callback_->response_.connection_info,
              HttpResponse::CONNECTION_INFO_HTTP1);
  }

  for (int i = 0; i < 2; ++i) {
    HttpRequest request;
    request.method = "GET";
    request.url = "https://example.com:4430";
    http_client_->Request(request);

    response_callback_->ResetSync();
    response_callback_->Wait();
    EXPECT_EQ(response_callback_->response_.connection_info,
              HttpResponse::CONNECTION_INFO_QUIC1_SDPY3);
  }
}

} // namespace stellite