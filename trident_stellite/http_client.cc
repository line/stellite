//
// 2016 write by snibug@linecorp.com
//

#include "http_client.h"

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

const char kMethodDelete[] = "DELETE";
const char kMethodGet[] = "GET";
const char kMethodHead[] = "HEAD";
const char kMethodPost[] = "POST";
const char kMethodPut[] = "PUT";

const int kDefaultRequestTimeout = 60 * 1000; // 60 seconds

class TRIDENT_STELLITE_EXPORT HttpResponseDelegate
    : public stellite::HttpResponseDelegate {
 public:
  HttpResponseDelegate(HttpClientVisitor* visitor)
      : visitor_(visitor) {
  }

  ~HttpResponseDelegate() override {}

  void OnHttpResponse(int request_id, const stellite::HttpResponse& response,
                      const char* data, size_t len) override {
    const std::string& raw_headers = response.headers.raw_headers();
    visitor_->OnHttpResponse(request_id,
                             response.response_code,
                             raw_headers.c_str(), raw_headers.size(),
                             data, len,
                             static_cast<int>(response.connection_info));
  }

  void OnHttpStream(int request_id, const stellite::HttpResponse& response,
                    const char* data, size_t len) override {
    const std::string& raw_headers = response.headers.raw_headers();
    visitor_->OnHttpResponse(request_id, response.response_code,
                             raw_headers.c_str(), raw_headers.size(),
                             data, len,
                             static_cast<int>(response.connection_info));
  }

  void OnHttpError(int request_id, int error_code,
                   const std::string& error_message) override {
    visitor_->OnError(request_id, error_code, error_message.c_str(),
                      error_message.size());
  }

 private:
  HttpClientVisitor* visitor_;
};

class TRIDENT_STELLITE_EXPORT HttpClient::HttpClientImpl {
 public:
  HttpClientImpl(HttpClient::Params& params, HttpClientVisitor* visitor)
      : visitor_(visitor),
        response_delegate_(new HttpResponseDelegate(visitor)) {
    InitInternal(params);
  }

  ~HttpClientImpl() {}

  int Request(HttpClient::RequestMethod method,
              const std::string& url,
              const std::string& raw_header,
              const std::string& body,
              int timeout) {
    stellite::HttpRequest request;
    request.url = url;

    switch (method) {
      case HttpClient::HTTP_GET:
        request.method = kMethodGet;
        break;
      case HttpClient::HTTP_POST:
        request.method = kMethodPost;
        break;
      case HttpClient::HTTP_PUT:
        request.method = kMethodPut;
        break;
      case HttpClient::HTTP_DELETE:
        request.method = kMethodDelete;
        break;
      case HttpClient::HTTP_HEAD:
        request.method = kMethodHead;
        break;
      default:
        request.method = kMethodGet;
        break;
    }

    request.headers.SetRawHeader(raw_header);

    if (body.size()) {
      request.upload_stream.write(body.c_str(), body.size());
    }

    return http_client_->Request(request, timeout);
  }

  HttpClientVisitor* client_visitor() {
    return visitor_;
  }

  stellite::HttpClientContext* client_context() {
    return context_.get();
  }

 private:
  void InitInternal(HttpClient::Params& params) {
    stellite::HttpClientContext::Params context_params;
    context_params.using_http2 = params.using_http2;
    context_params.using_quic = params.using_quic;
    context_params.using_spdy = params.using_spdy;

    context_.reset(new stellite::HttpClientContext(context_params));
    if (!context_->Initialize()) {
      LLOG(ERROR) << "context initialize are failed";
      return;
    }

    // create internal client
    http_client_ = context_->CreateHttpClient(response_delegate_.get());
  }

  void TeardownInternal() {
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

  HttpClientVisitor* visitor_;
  std::unique_ptr<stellite::HttpClientContext> context_;
  std::unique_ptr<HttpResponseDelegate> response_delegate_;

  stellite::HttpClient* http_client_;
};

HttpClient::HttpClient(Params params, HttpClientVisitor* visitor)
    : impl_(new HttpClientImpl(params, visitor)) {
}

HttpClient::~HttpClient() {
  delete impl_;
}

int HttpClient::Request(RequestMethod method,
                        const char* raw_url, size_t url_len,
                        const char* raw_header, size_t header_len,
                        const char* raw_body, size_t body_len) {
  return Request(method, raw_url, url_len, raw_header, header_len, raw_body,
                 body_len, kDefaultRequestTimeout);
}

int HttpClient::Request(RequestMethod method,
                        const char* raw_url, size_t url_len,
                        const char* raw_header, size_t header_len,
                        const char* raw_body, size_t body_len, int timeout) {
  std::string url(raw_url, url_len);
  std::string header(raw_header, header_len);
  std::string body(raw_body, body_len);
  return impl_->Request(method, url, header, body, timeout);
}

void HttpClient::CancelAll() {
  impl_->client_context()->Cancel();
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
