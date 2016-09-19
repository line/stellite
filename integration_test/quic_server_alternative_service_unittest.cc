//
// Copyright 2016 LINE Corporation
//

#include "base/synchronization/waitable_event.h"
#include "stellite/include/http_client.h"
#include "stellite/include/http_client_context.h"
#include "stellite/include/http_request.h"
#include "stellite/include/http_response.h"
#include "stellite/test/http_test_server.h"
#include "stellite/test/quic_mock_server.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace stellite {

class AlternativeServiceCallback : public HttpResponseDelegate {
 public:
  AlternativeServiceCallback()
      : on_response_received_(true, false) {
  }

  void OnHttpResponse(int request_id, const HttpResponse& response,
                      const char* data, size_t len) override {
    connection_info_ = response.connection_info;
    on_response_received_.Signal();
  }

  void OnHttpStream(int request_id, const HttpResponse& response,
                    const char* data, size_t len) override {
    on_response_received_.Signal();
  }

  void OnHttpError(int request_id, int error_code,
                   const std::string& error_message) override {
    on_response_received_.Signal();
  }

  void Wait() {
    on_response_received_.Wait();
  }

  HttpResponse::ConnectionInfo connection_info() {
    return connection_info_;
  }

  base::WaitableEvent on_response_received_;
  HttpResponse::ConnectionInfo connection_info_;
};


class QuicServerAlternativeServiceTest : public testing::Test {
 public:
  QuicServerAlternativeServiceTest() {}

  void SetUp() override {
    // QUIC server
  quic_server_.reset(new QuicMockServer());
  quic_server_->SetProxyPass("http://example.com:9999");
  quic_server_->SetCertificate("example.com.cert.pkcs8",
                               "example.com.cert.pem");
  quic_server_->Start(4430);

  // HTTP backend server
  HttpTestServer::Params backend_params;
  backend_params.port = 9999;
  http_backend_server_.reset(new HttpTestServer(backend_params));
  http_backend_server_->Start();

  // HTTP frontend server
  HttpTestServer::Params frontend_params;
  frontend_params.port = 8888;
  frontend_params.server_type = HttpTestServer::TYPE_HTTPS;
  frontend_params.response_delay = 50;
  frontend_params.alternative_services =
      "quic=\":4430\"";
    http_frontend_server_.reset(new HttpTestServer(frontend_params));
    http_frontend_server_->Start();

    // HTTP client
    HttpClientContext::Params client_context_params;
    client_context_params.using_quic = true;

    http_client_context_.reset(new HttpClientContext(client_context_params));
    http_client_context_->Initialize();

    response_callback_.reset(new AlternativeServiceCallback());
    http_client_ = http_client_context_->CreateHttpClient(
        response_callback_.get());
  }

  void TearDown() override {
    http_client_context_->ReleaseHttpClient(http_client_);
    http_backend_server_->Shutdown();
    http_frontend_server_->Shutdown();
    quic_server_->Shutdown();
  }

  std::unique_ptr<QuicMockServer> quic_server_;
  std::unique_ptr<HttpTestServer> http_backend_server_;
  std::unique_ptr<HttpTestServer> http_frontend_server_;
  std::unique_ptr<HttpClientContext> http_client_context_;
  std::unique_ptr<AlternativeServiceCallback> response_callback_;

  HttpClient* http_client_;
};

TEST_F(QuicServerAlternativeServiceTest, AlternativeServiceDiscovery) {
  for (int i = 0; i < 3; ++i) {
    HttpRequest request;
    request.method = "GET";
    request.url = "https://example.com:8888/pending?delay=50";
    http_client_->Request(request);

    response_callback_->Wait();

    if (i < 2) {
      EXPECT_EQ(response_callback_->connection_info(),
                HttpResponse::ConnectionInfo::CONNECTION_INFO_HTTP1);
    } else {
      EXPECT_EQ(response_callback_->connection_info(),
                HttpResponse::ConnectionInfo::CONNECTION_INFO_QUIC1_SDPY3);
    }
  }
}

} // namespace stellite