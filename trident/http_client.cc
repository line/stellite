//
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

#include "trident/include/http_client.h"

#include <string>
#include <memory>

#include "stellite/include/http_client.h"
#include "stellite/include/http_client_context.h"
#include "stellite/include/http_request.h"
#include "stellite/include/http_response.h"
#include "stellite/include/logging.h"

#if defined(ANDROID)
#include "base/android/base_jni_registrar.h"
#include "base/android/context_utils.h"
#include "base/android/jni_android.h"
#include "base/android/jni_utils.h"
#include "net/android/net_jni_registrar.h"
#endif

namespace trident {
class HttpClientVisitor;

class TRIDENT_EXPORT HttpResponseDelegate
    : public stellite::HttpResponseDelegate {
 public:
  HttpResponseDelegate(HttpClientVisitor* visitor);
  ~HttpResponseDelegate() override;

  void OnHttpResponse(int request_id, const stellite::HttpResponse& response,
                      const char* data, size_t len) override;
  void OnHttpStream(int request_id, const stellite::HttpResponse& response,
                    const char* data, size_t len, bool fin) override;
  void OnHttpError(int request_id, int error_code,
                   const std::string& error_message) override;
 private:
  HttpClientVisitor* visitor_;
};

class TRIDENT_EXPORT HttpClient::ClientImpl {
 public:
  ClientImpl(HttpClient::Params& params, HttpClientVisitor* visitor);
  ~ClientImpl();

  int Request(HttpClient::RequestMethod method, const std::string& url,
              const std::string& raw_header, const std::string& body,
              bool chunked_upload, bool stream_response, int timeout);

  HttpClientVisitor* client_visitor() {
    return visitor_;
  }

  stellite::HttpClientContext* client_context() {
    return context_.get();
  }

 private:
  void InitInternal(HttpClient::Params& params);
  void TeardownInternal();

  HttpClientVisitor* visitor_;
  std::unique_ptr<stellite::HttpClientContext> context_;
  std::unique_ptr<HttpResponseDelegate> response_delegate_;

  stellite::HttpClient* http_client_;
};

HttpResponseDelegate::HttpResponseDelegate(HttpClientVisitor* visitor)
  : visitor_(visitor) {
}

HttpResponseDelegate::~HttpResponseDelegate() {}

void HttpResponseDelegate::OnHttpResponse(
    int request_id, const stellite::HttpResponse& response,
    const char* data, size_t len) {
  const std::string& raw_headers = response.headers.raw_headers();
  visitor_->OnHttpResponse(request_id,
                           response.response_code,
                           raw_headers.c_str(), raw_headers.size(),
                           data, len,
                           static_cast<int>(response.connection_info));
}

void HttpResponseDelegate::OnHttpStream(
    int request_id, const stellite::HttpResponse& response,
    const char* data, size_t len, bool fin) {
  const std::string& raw_headers = response.headers.raw_headers();
  visitor_->OnHttpStream(request_id, response.response_code,
                         raw_headers.c_str(), raw_headers.size(),
                         data, len, fin,
                         static_cast<int>(response.connection_info));
}

void HttpResponseDelegate::OnHttpError(int request_id, int error_code,
                                       const std::string& error_message) {
  visitor_->OnError(request_id, error_code, error_message.c_str(),
                    error_message.size());
}

HttpClient::ClientImpl::ClientImpl(HttpClient::Params& params,
                                   HttpClientVisitor* visitor)
  : visitor_(visitor),
    response_delegate_(new HttpResponseDelegate(visitor)) {
  InitInternal(params);
}

HttpClient::ClientImpl::~ClientImpl() {
  TeardownInternal();
}

int HttpClient::ClientImpl::Request(HttpClient::RequestMethod method,
                                    const std::string& url,
                                    const std::string& raw_header,
                                    const std::string& body,
                                    bool chunked_upload,
                                    bool stream_response,
                                    int timeout) {
  stellite::HttpRequest request;
  request.url = url;

  switch (method) {
    case HttpClient::HTTP_GET:
      request.request_type = stellite::HttpRequest::GET;
      break;
    case HttpClient::HTTP_POST:
      request.request_type = stellite::HttpRequest::POST;
      break;
    case HttpClient::HTTP_PUT:
      request.request_type = stellite::HttpRequest::PUT;
      break;
    case HttpClient::HTTP_DELETE:
      request.request_type = stellite::HttpRequest::DELETE_REQUEST;
      break;
    case HttpClient::HTTP_HEAD:
      request.request_type = stellite::HttpRequest::HEAD;
      break;
    default:
      request.request_type = stellite::HttpRequest::GET;
      break;
  }

  request.headers.SetRawHeader(raw_header);

  if (body.size()) {
    request.upload_stream.write(body.c_str(), body.size());
  }

  return http_client_->Request(request, timeout);
}


void HttpClient::ClientImpl::InitInternal(HttpClient::Params& params) {
  stellite::HttpClientContext::Params context_params;
  context_params.using_http2 = params.using_http2;
  context_params.using_quic = params.using_quic;

  context_.reset(new stellite::HttpClientContext(context_params));
  if (!context_->Initialize()) {
    LLOG(ERROR) << "context initialize are failed";
    return;
  }

  // create internal client
  http_client_ = context_->CreateHttpClient(response_delegate_.get());
}

void HttpClient::ClientImpl::TeardownInternal() {
  if (!context_.get()) {
    LLOG(ERROR) << "context is not initialized error";
    return;
  }

  // release internal client
  context_->ReleaseHttpClient(http_client_);

  if (!context_->TearDown()) {
    LLOG(ERROR) << "context teardown are failed";
    return;
  }

  http_client_ = nullptr;
}

HttpClient::HttpClient(Params params, HttpClientVisitor* visitor)
    : impl_(new ClientImpl(params, visitor)) {
}

HttpClient::~HttpClient() {
  delete impl_;
}

int HttpClient::Request(RequestMethod method,
                        const char* raw_url, size_t url_len,
                        const char* raw_header, size_t header_len,
                        const char* raw_body, size_t body_len,
                        bool chunked_upload, bool stream_response,
                        int timeout) {
  std::string url(raw_url, url_len);
  std::string header(raw_header, header_len);
  std::string body(raw_body, body_len);
  return impl_->Request(method, url, header, body, chunked_upload,
                        stream_response, timeout);
}

void HttpClient::CancelAll() {
  impl_->client_context()->CancelAll();
}

#if defined(ANDROID)

  // static
void ClientContext::InitVM(JavaVM* vm) {
  if (base::android::IsVMInitialized()) {
    return;
  }

  // initialize
  base::android::InitVM(vm);

  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::InitReplacementClassLoader(
      env, base::android::GetClassLoader(env));

  if (!base::android::RegisterJni(env)) {
    return;
  }

  if (!net::android::RegisterJni(env)) {
    return;
  }
}

#endif

// static
int ClientContext::GetMinLogLevel() {
  return stellite::GetMinLogLevel();
}

// static
void ClientContext::SetMinLogLevel(int level) {
  stellite::SetMinLogLevel(level);
}

}  // namespace trident
