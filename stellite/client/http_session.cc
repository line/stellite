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

#include "stellite/include/http_session.h"

#include <string>
#include <memory>

#include "base/logging.h"
#include "base/macros.h"
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

namespace stellite {

// SessionResponseDelegate -----------------------------------------------------

class STELLITE_EXPORT SessionResponseDelegate : public HttpResponseDelegate {
 public:
  SessionResponseDelegate(HttpSessionVisitor* visitor);
  ~SessionResponseDelegate() override;

  void OnHttpResponse(int request_id, const stellite::HttpResponse& response,
                      const char* data, size_t len) override;

  void OnHttpStream(int request_id, const stellite::HttpResponse& response,
                    const char* data, size_t len, bool fin) override;

  void OnHttpError(int request_id, int error_code,
                   const std::string& error_message) override;
 private:
  HttpSessionVisitor* visitor_;

  DISALLOW_COPY_AND_ASSIGN(SessionResponseDelegate);
};

SessionResponseDelegate::SessionResponseDelegate(HttpSessionVisitor* visitor)
  : visitor_(visitor) {
}

SessionResponseDelegate::~SessionResponseDelegate() {}

void SessionResponseDelegate::OnHttpResponse(
    int request_id, const stellite::HttpResponse& response,
    const char* data, size_t len) {
  const std::string& raw_headers = response.headers.raw_headers();
  visitor_->OnHttpResponse(request_id,
                           response.response_code,
                           raw_headers.c_str(), raw_headers.size(),
                           data, len,
                           static_cast<int>(response.connection_info));
}

void SessionResponseDelegate::OnHttpStream(
    int request_id, const stellite::HttpResponse& response,
    const char* data, size_t len, bool fin) {
  const std::string& raw_headers = response.headers.raw_headers();
  visitor_->OnHttpStream(request_id, response.response_code,
                         raw_headers.c_str(), raw_headers.size(),
                         data, len, fin,
                         static_cast<int>(response.connection_info));
}

void SessionResponseDelegate::OnHttpError(int request_id, int error_code,
                                          const std::string& error_message) {
  visitor_->OnError(request_id, error_code, error_message.c_str(),
                    error_message.size());
}

// HttpSessionImpl::SessionImpl ------------------------------------------------

class STELLITE_EXPORT HttpSession::SessionImpl {
 public:
  SessionImpl(HttpSession::Params& params, HttpSessionVisitor* visitor);
  ~SessionImpl();

  int Request(HttpSession::RequestMethod method, const std::string& url,
              const std::string& raw_header, const std::string& body,
              bool chunked_upload, bool stream_response, int timeout);

  HttpSessionVisitor* client_visitor() {
    return visitor_;
  }

  HttpClientContext* client_context() {
    return context_.get();
  }

 private:
  void InitInternal(HttpSession::Params& params);
  void TeardownInternal();

  HttpSessionVisitor* visitor_;
  std::unique_ptr<HttpClientContext> context_;
  std::unique_ptr<HttpResponseDelegate> response_delegate_;

  HttpClient* http_client_;
};

HttpSession::SessionImpl::SessionImpl(HttpSession::Params& params,
                                      HttpSessionVisitor* visitor)
    : visitor_(visitor),
      response_delegate_(new SessionResponseDelegate(visitor)) {
  InitInternal(params);
}

HttpSession::SessionImpl::~SessionImpl() {
  TeardownInternal();
}

int HttpSession::SessionImpl::Request(HttpSession::RequestMethod method,
                                      const std::string& url,
                                      const std::string& raw_header,
                                      const std::string& body,
                                      bool chunked_upload,
                                      bool stream_response,
                                      int timeout) {
  stellite::HttpRequest request;
  request.url = url;
  request.is_chunked_upload = chunked_upload;
  request.is_stream_response = stream_response;

  switch (method) {
    case HttpSession::HTTP_GET:
      request.request_type = stellite::HttpRequest::GET;
      break;
    case HttpSession::HTTP_POST:
      request.request_type = stellite::HttpRequest::POST;
      break;
    case HttpSession::HTTP_PUT:
      request.request_type = stellite::HttpRequest::PUT;
      break;
    case HttpSession::HTTP_DELETE:
      request.request_type = stellite::HttpRequest::DELETE_REQUEST;
      break;
    case HttpSession::HTTP_HEAD:
      request.request_type = stellite::HttpRequest::HEAD;
      break;
    default:
      request.request_type = stellite::HttpRequest::GET;
      break;
  }

  request.headers.SetRawHeader(raw_header);

  if (body.size()) {
    if (chunked_upload) {
      LOG(WARNING) << "request's payload are ignored at chunked upload";
    } else {
      request.upload_stream.write(body.c_str(), body.size());
    }
  }

  return http_client_->Request(request, timeout);
}


void HttpSession::SessionImpl::InitInternal(HttpSession::Params& params) {
  HttpClientContext::Params context_params;
  context_params.using_http2 = params.using_http2;
  context_params.using_quic = params.using_quic;

  context_.reset(new HttpClientContext(context_params));
  if (!context_->Initialize()) {
    LOG(ERROR) << "context initialize are failed";
    return;
  }

  // create internal client
  http_client_ = context_->CreateHttpClient(response_delegate_.get());
}

void HttpSession::SessionImpl::TeardownInternal() {
  if (!context_.get()) {
    LOG(ERROR) << "context is not initialized error";
    return;
  }

  // release internal client
  context_->ReleaseHttpClient(http_client_);

  if (!context_->TearDown()) {
    LOG(ERROR) << "context teardown are failed";
    return;
  }

  http_client_ = nullptr;
}

HttpSession::HttpSession(Params params, HttpSessionVisitor* visitor)
    : impl_(new SessionImpl(params, visitor)) {
}

HttpSession::~HttpSession() {
  if (impl_) {
    delete impl_;
    impl_ = nullptr;
  }
}

int HttpSession::Request(RequestMethod method,
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

void HttpSession::CancelAll() {
  impl_->client_context()->CancelAll();
}

#if defined(ANDROID)

  // static
void HttpSessionContext::InitVM(JavaVM* vm) {
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
int HttpSessionContext::GetMinLogLevel() {
  return stellite::GetMinLogLevel();
}

// static
void HttpSessionContext::SetMinLogLevel(int level) {
  stellite::SetMinLogLevel(level);
}

}  // namespace stellite
