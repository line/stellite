//
// 2016 write by snibug@linecorp.com
//

#ifndef TRIDENT_STELLITE_HTTP_CLIENT_H_
#define TRIDENT_STELLITE_HTTP_CLIENT_H_

#include <stddef.h>

#if defined(ANDROID)
#include <jni.h>
#endif

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(TRIDENT_STELLITE_IMPLEMENTATION)
#define TRIDENT_STELLITE_EXPORT __declspec(dllexport)
#else
#define TRIDENT_STELLITE_EXPORT __declspec(dllimport)
#endif  // TRIDENT_STELLITE_IMPLEMENTATION

#else  // defined(WIN32)
#if defined(TRIDENT_STELLITE_IMPLEMENTATION)
#define TRIDENT_STELLITE_EXPORT __attribute__((visibility("default")))
#else
#define TRIDENT_STELLITE_EXPORT
#endif  // TRIDENT_STELLITE_IMPLEMENTATION
#endif

#else  // COMPONENT_BUILD
#define TRIDENT_STELLITE_EXPORT
#endif

namespace trident {

class TRIDENT_STELLITE_EXPORT HttpClientVisitor {
 public:
  enum ConnectionInfo {
    CONNECTION_INFO_UNKNOWN = 0,
    CONNECTION_INFO_HTTP1 = 1,
    CONNECTION_INFO_DEPRECATED_SPDY2 = 2,
    CONNECTION_INFO_SPDY3 = 3,
    CONNECTION_INFO_HTTP2 = 4,
    CONNECTION_INFO_QUIC1_SDPY3 = 5,
    CONNECTION_INFO_HTTP2_14 = 6,
    CONNECTION_INFO_HTTP2_15 = 7,
    NUM_OF_CONNECTION_INFOS,
  };

  virtual ~HttpClientVisitor() {}

  // caution: raw_header contain null value that was working for header
  // delimitter and linking a carriage return('\r\n')
  virtual void OnHttpResponse(int request_id, int responspe_code,
                              const char* raw_header, size_t header_len,
                              const char* body, size_t body_len,
                              int connection_info) = 0;

  virtual void OnError(int request_id, int error_code,
                       const char* reason, size_t len)=0;
};

class TRIDENT_STELLITE_EXPORT HttpClient {
 public:
  enum RequestMethod {
    HTTP_DELETE,
    HTTP_GET,
    HTTP_HEAD,
    HTTP_POST,
    HTTP_PUT,
  };

  struct Params {
    bool using_http2;
    bool using_quic;
    bool using_spdy;
  };

  explicit HttpClient(Params params, HttpClientVisitor* visitor);
  virtual ~HttpClient();

  // caution: raw_header delimiter must \r\n
  int Request(RequestMethod method,
              const char* url, size_t url_len,
              const char* raw_header, size_t header_len,
              const char* body, size_t body_len);

  int Request(RequestMethod method,
              const char* url, size_t url_len,
              const char* raw_header, size_t header_len,
              const char* body, size_t body_len, int timeout);

  void CancelAll();

 private:
  class HttpClientImpl;
  HttpClientImpl* impl_;

  // disallow copy and assign
  HttpClient(const HttpClient&);
  void operator=(const HttpClient&);
};

class TRIDENT_STELLITE_EXPORT ClientContext {
 public:
  enum LogLevel {
    LOGGING_INFO = 0,
    LOGGING_WARN = 1,
    LOGGING_ERROR = 2,
    LOGGING_FATAL = 3,
  };

  static int GetMinLogLevel();
  static void SetMinLogLevel(int level);

#if defined(ANDROID)
  static void InitVM(JavaVM* vm);
#endif
};


}  // namespace trident

#endif  // TRIDENT_STELLITE_HTTP_CLIENT_H_
