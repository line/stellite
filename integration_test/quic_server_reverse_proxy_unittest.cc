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
#include "testing/gtest/include/gtest/gtest.h"

namespace stellite {

class ReverseProxyCallback : public HttpResponseDelegate {
 public:
  ReverseProxyCallback()
      : on_response_received_(true, false) {
  }

  void OnHttpResponse(int request_id, const HttpResponse& response,
                      const char* data, size_t len) override {
    response_body_.append(data, len);
    on_response_received_.Signal();
  }

  void OnHttpStream(int request_id, const HttpResponse& response,
                    const char* data, size_t len) override {
    response_body_.append(data, len);
    on_response_received_.Signal();
  }

  void OnHttpError(int request_id, int error_code,
                   const std::string& error_message) override {
    on_response_received_.Signal();
  }

  void Wait() {
    on_response_received_.Wait();
  }

  base::WaitableEvent on_response_received_;
  std::string response_body_;
};

class QuicServerReverseProxyTest : public testing::Test {
 public:
  QuicServerReverseProxyTest() {
  }

  void SetUp() override {
    quic_server_.reset(new QuicMockServer());
    quic_server_->SetProxyPass("http://example.com:9999");
    quic_server_->SetCertificate("example.com.cert.pkcs8",
                                 "example.com.cert.pem");
    quic_server_->Start(4430);

    HttpTestServer::Params backend_params;
    backend_params.port = 9999;
    http_backend_server_.reset(new HttpTestServer(backend_params));
    http_backend_server_->Start();

    HttpClientContext::Params client_context_params;
    client_context_params.using_quic = true;
    client_context_params.origin_to_force_quic_on = "example.com:4430";

    http_client_context_.reset(new HttpClientContext(client_context_params));
    http_client_context_->Initialize();

    response_callback_.reset(new ReverseProxyCallback());

    http_client_ = http_client_context_->CreateHttpClient(
        response_callback_.get());
  }

  void TearDown() override {
    http_client_context_->ReleaseHttpClient(http_client_);
    http_backend_server_->Shutdown();
    quic_server_->Shutdown();
  }

  std::unique_ptr<QuicMockServer> quic_server_;
  std::unique_ptr<HttpTestServer> http_backend_server_;
  std::unique_ptr<HttpClientContext> http_client_context_;
  std::unique_ptr<ReverseProxyCallback> response_callback_;

  HttpClient* http_client_;
};


TEST_F(QuicServerReverseProxyTest, ProxyPassToAnotherPortNumber) {
  HttpRequest request;
  request.method = "GET";
  request.url = "https://example.com:4430/";
  http_client_->Request(request);

  response_callback_->Wait();
  EXPECT_EQ(response_callback_->response_body_, "get");
}

} // Stellite