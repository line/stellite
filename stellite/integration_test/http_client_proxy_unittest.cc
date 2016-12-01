//
// Copyright 2016 LINE Corporation
//

#include "stellite/include/http_client.h"

#include "base/memory/scoped_ptr.h"
#include "base/synchronization/waitable_event.h"
#include "stellite/include/http_client.h"
#include "stellite/include/http_client_context.h"
#include "stellite/include/http_request.h"
#include "stellite/include/http_response.h"
#include "stellite/test/http_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"


namespace stellite {

class HttpProxyResponseCallback : public HttpResponseDelegate {
 public:
  HttpProxyResponseCallback()
      : on_response_completed_(true, false) {
  }

  void OnHttpResponse(const HttpResponse& response,
                      const char* data, size_t len) override {
    response_body_.assign(data, len);
    on_response_completed_.Signal();
  }

  void OnHttpStream(const HttpResponse& response,
                    const char* data, size_t len) override {
    on_response_completed_.Signal();
  }

  void OnHttpError(int error_code, const std::string& message) override {
    on_response_completed_.Signal();
  }

  void WaitForResponse() {
    on_response_completed_.Wait();
  }

  std::string response_body() {
    return response_body_;
  }

  base::WaitableEvent on_response_completed_;
  std::string response_body_;
};

class HttpProxyTest : public testing::Test {
 public:
  HttpProxyTest() {
  }

  void SetUp() override {
    HttpTestServer::Params proxy_params;
    proxy_params.server_type = HttpTestServer::TYPE_PROXY;
    proxy_params.port = 19000;
    proxy_server_.reset(new HttpTestServer(proxy_params));
    proxy_server_->Start();

    HttpTestServer::Params backend_params;
    backend_params.server_type = HttpTestServer::TYPE_HTTP;
    backend_params.port = 18000;
    backend_server_.reset(new HttpTestServer(backend_params));
    backend_server_->Start();

    HttpClientContext::Params params;
    params.using_quic = true;
    params.using_spdy = true;
    params.using_http2 = true;
    params.proxy_host = "http://example.com:19000";

    context_.reset(new HttpClientContext(params));
    context_->Initialize();

    client_.reset(context_->CreateHttpClient());
  }

  void TearDown() override {
    context_->ReleaseHttpClient(client_.release());
    context_->TearDown();

    backend_server_->Shutdown();
    proxy_server_->Shutdown();
  }

  scoped_ptr<HttpTestServer> proxy_server_;
  scoped_ptr<HttpTestServer> backend_server_;
  scoped_ptr<HttpClientContext> context_;
  scoped_ptr<HttpClient> client_;
};

TEST_F(HttpProxyTest, HttpProxyRequest) {
  HttpRequest request;
  request.url = "http://example.com:18000";
  request.method = "GET";

  sp<HttpResponseDelegate> delegate(new HttpProxyResponseCallback());
  client_->Request(request, delegate);

  HttpProxyResponseCallback* callback =
      static_cast<HttpProxyResponseCallback*>(delegate.get());
  callback->WaitForResponse();

  EXPECT_EQ(callback->response_body(), "get");
}

} // Stellite