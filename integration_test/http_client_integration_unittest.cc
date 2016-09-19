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

class HttpIntegrationResponseCallback : public HttpResponseDelegate {
 public:
  HttpIntegrationResponseCallback()
      : on_request_completed_(true, false),
        request_completed_(false),
        error_code_(0) {
  }

  void OnHttpResponse(int request_id, const HttpResponse& response,
                      const char* data, size_t len) override {
    request_completed_ = true;
    data_ = std::string(data, len);
    on_request_completed_.Signal();
  }

  void OnHttpStream(int request_id, const HttpResponse& response,
                    const char* stream_data, size_t len) override {
  }

  void OnHttpError(int request_id, int error_code,
                   const std::string& message) override {
    request_completed_ = true;
    error_code_ = error_code;
    on_request_completed_.Signal();
  }

  void WaitForResponse() {
    on_request_completed_.Wait();
  }

  base::WaitableEvent on_request_completed_;
  bool request_completed_;
  int error_code_;
  std::string data_;
};

class HttpClientIntegrationTest : public testing::Test {
 public:
  void RunHttpRequest(HttpTestServer::ServerType server_type,
                      HttpTestServer::RequestType request_type) {
    HttpTestServer::Params server_params;
    server_params.server_type = server_type;
    server_params.port = 18000;

    std::unique_ptr<HttpTestServer> server(new HttpTestServer(server_params));
    EXPECT_TRUE(server->Start());

    HttpClientContext::Params client_params;
    switch (server_type) {
      case HttpTestServer::TYPE_HTTP:
      case HttpTestServer::TYPE_HTTPS:
        break;
      case HttpTestServer::TYPE_HTTP2:
        client_params.using_http2 = true;
        break;
      case HttpTestServer::TYPE_SPDY31:
        client_params.using_spdy = true;
        break;
      case HttpTestServer::TYPE_QUIC:
        client_params.using_quic = true;
        break;
      default:
        LOG(ERROR) << "Unknown server type";
        break;
    }

    std::unique_ptr<HttpClientContext> client_context(
        new HttpClientContext(client_params));
    EXPECT_TRUE(client_context->Initialize());

    HttpRequest request;
    if (server_type == HttpTestServer::TYPE_HTTP) {
      request.url = "http://example.com:18000";
    } else {
      request.url = "https://example.com:18000";
    }

    switch (request_type) {
      case HttpTestServer::GET:
        request.method = "GET";
        break;
      case HttpTestServer::POST:
        request.method = "POST";
        break;
      case HttpTestServer::PUT:
        request.method = "PUT";
        break;
      case HttpTestServer::DELETE:
        request.method = "DELETE";
        break;
      default:
        LOG(ERROR) << "Unknown request";
        break;
    }

    if (request_type == HttpTestServer::POST) {
      std::string upload_body = "hello world";
      request.upload_stream.write(upload_body.c_str(), upload_body.size());
    }

    std::unique_ptr<HttpIntegrationResponseCallback> response_callback(
        new HttpIntegrationResponseCallback());

    HttpClient* client = client_context->CreateHttpClient(
        response_callback.get());
    client->Request(request);

    response_callback->WaitForResponse();

    EXPECT_EQ(response_callback->error_code_, 0);
    EXPECT_TRUE(response_callback->request_completed_);

    client_context->ReleaseHttpClient(client);
    EXPECT_TRUE(client_context->TearDown());

    server->Shutdown();
  }
};

TEST_F(HttpClientIntegrationTest, HttpGetTest) {
  RunHttpRequest(HttpTestServer::TYPE_HTTP, HttpTestServer::GET);
}

TEST_F(HttpClientIntegrationTest, HttpPostTest) {
  RunHttpRequest(HttpTestServer::TYPE_HTTP, HttpTestServer::POST);
}

TEST_F(HttpClientIntegrationTest, HttpPutTest) {
  RunHttpRequest(HttpTestServer::TYPE_HTTP, HttpTestServer::PUT);
}

TEST_F(HttpClientIntegrationTest, HttpDeleteTest) {
  RunHttpRequest(HttpTestServer::TYPE_HTTP, HttpTestServer::DELETE);
}

TEST_F(HttpClientIntegrationTest, HttpsGetTest) {
  RunHttpRequest(HttpTestServer::TYPE_HTTPS, HttpTestServer::GET);
}

TEST_F(HttpClientIntegrationTest, HttpsPostTest) {
  RunHttpRequest(HttpTestServer::TYPE_HTTPS, HttpTestServer::POST);
}

TEST_F(HttpClientIntegrationTest, HttpsPutTest) {
  RunHttpRequest(HttpTestServer::TYPE_HTTPS, HttpTestServer::PUT);
}

TEST_F(HttpClientIntegrationTest, HttpsDeleteTest) {
  RunHttpRequest(HttpTestServer::TYPE_HTTPS, HttpTestServer::DELETE);
}

TEST_F(HttpClientIntegrationTest, SpdyGetTest) {
  RunHttpRequest(HttpTestServer::TYPE_SPDY31, HttpTestServer::GET);
}

TEST_F(HttpClientIntegrationTest, SpdyPostTest) {
  RunHttpRequest(HttpTestServer::TYPE_SPDY31, HttpTestServer::POST);
}

TEST_F(HttpClientIntegrationTest, SpdyPutTest) {
  RunHttpRequest(HttpTestServer::TYPE_SPDY31, HttpTestServer::PUT);
}

TEST_F(HttpClientIntegrationTest, SpdyDeleteTest) {
  RunHttpRequest(HttpTestServer::TYPE_SPDY31, HttpTestServer::DELETE);
}

TEST_F(HttpClientIntegrationTest, Http2GetTest) {
  RunHttpRequest(HttpTestServer::TYPE_HTTP2, HttpTestServer::GET);
}

TEST_F(HttpClientIntegrationTest, ClientResetWhenRequestAreProcessing) {
  HttpTestServer::Params server_params;
  server_params.server_type = HttpTestServer::TYPE_HTTP;
  server_params.port = 18000;

  std::unique_ptr<HttpTestServer> server(new HttpTestServer(server_params));
  EXPECT_TRUE(server->Start());

  HttpClientContext::Params client_params;
  client_params.using_quic = false;
  client_params.using_spdy = false;
  client_params.using_http2 = false;

  std::unique_ptr<HttpClientContext> client_context(
      new HttpClientContext(client_params));
  EXPECT_TRUE(client_context->Initialize());

  HttpRequest request;
  request.url = "http://example.com:18000/pending?delay=10000";
  request.method = "GET";

  std::unique_ptr<HttpIntegrationResponseCallback> response_callback(
      new HttpIntegrationResponseCallback());
  HttpClient* client(client_context->CreateHttpClient(response_callback.get()));
  client->Request(request);

  EXPECT_FALSE(response_callback->request_completed_);

  client_context->ReleaseHttpClient(client);
  EXPECT_TRUE(client_context->TearDown());
  server->Shutdown();
}

TEST_F(HttpClientIntegrationTest, UploadStream) {
  HttpTestServer::Params server_params;
  server_params.server_type = HttpTestServer::TYPE_HTTP;
  server_params.port = 18000;

  std::unique_ptr<HttpTestServer> server(new HttpTestServer(server_params));
  EXPECT_TRUE(server->Start());

  HttpClientContext::Params client_params;
  std::unique_ptr<HttpClientContext> client_context(
      new HttpClientContext(client_params));
  EXPECT_TRUE(client_context->Initialize());

  HttpRequest request;
  request.url = "http://example.com:18000/pending?delay=10000";
  request.method = "POST";

  std::unique_ptr<HttpIntegrationResponseCallback> response_callback(
    new HttpIntegrationResponseCallback());

  HttpClient* client =
      client_context->CreateHttpClient(response_callback.get());
  client->Request(request);

  EXPECT_FALSE(response_callback->request_completed_);

  client_context->ReleaseHttpClient(client);
  EXPECT_TRUE(client_context->TearDown());
  server->Shutdown();
}

} // namespace stellite

