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

#ifndef STELLITE_INCLUDE_NOSTL_CLIENT_H_
#define STELLITE_INCLUDE_NOSTL_CLIENT_H_

#include <stddef.h>
#include <stdint.h>

#include "stellite_export.h"

#if defined(ANDROID)
#include <jni.h>
#endif

namespace stellite {

class STELLITE_EXPORT HttpSessionVisitor {
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

  virtual ~HttpSessionVisitor() {}

  // caution: raw_header contain null value that was working for header
  // delimitter and linking a carriage return('\r\n')
  virtual void OnHttpResponse(int request_id, int responspe_code,
                              const char* raw_header, size_t header_len,
                              const char* body, size_t body_len,
                              int connection_info) = 0;

  virtual void OnHttpStream(int request_id, int response_code,
                            const char* raw_header, size_t header_len,
                            const char* stream, size_t stream_len,
                            bool is_last_stream, int connection_info) = 0;

  virtual void OnError(int request_id, int error_code,
                       const char* reason, size_t len) = 0;
};

class STELLITE_EXPORT HttpSession {
 public:
  enum RequestMethod {
    HTTP_DELETE,
    HTTP_GET,
    HTTP_HEAD,
    HTTP_POST,
    HTTP_PUT,
  };

  HttpSession();
  virtual ~HttpSession();

  bool Start(HttpSessionVisitor* visitor);
  bool AddQuicHostToDirectRequestOn(const char* hostname, size_t len, uint16_t port);
  bool UsingHttp2(bool use);
  bool UsingQuic(bool use);

  // caution: raw_header delimiter must \r\n
  // if chunked_upload are true body and body_len are ignored
  int Request(RequestMethod method,
              const char* url, size_t url_len,
              const char* raw_header, size_t header_len,
              const char* body, size_t body_len, bool chunked_upload,
              bool stream_response, int timeout);

  bool AppendChunkToUpload(int request_id, const char* chunk, size_t len,
                           bool is_last);

  void CancelAll();

 private:
  class SessionImpl;
  SessionImpl* impl_;

  // disallow copy and assign
  HttpSession(const HttpSession&);
  void operator=(const HttpSession&);
};

class STELLITE_EXPORT HttpSessionContext {
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

}  // namespace stellite

#endif  // STELLITE_INCLUDE_NOSTL_CLIENT_H_
