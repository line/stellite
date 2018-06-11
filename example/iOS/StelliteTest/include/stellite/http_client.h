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

#ifndef QUIC_INCLUDE_HTTP_CLIENT_H_
#define QUIC_INCLUDE_HTTP_CLIENT_H_

#include <string>

#include "stellite_export.h"

namespace stellite {
struct HttpRequest;
struct HttpResponse;

// Interface of HTTP response or error callback
class STELLITE_EXPORT HttpResponseDelegate {
 public:
  virtual ~HttpResponseDelegate() {}

  virtual void OnHttpResponse(int request_id, const HttpResponse& response,
                              const char* body, size_t body_len) = 0;

  virtual void OnHttpStream(int request_id, const HttpResponse& response,
                            const char* stream, size_t stream_len,
                            bool is_last) = 0;

  // The error code are defined at net/base/net_error_list.h
  virtual void OnHttpError(int request_id, int error_code,
                           const std::string& error_message) = 0;
};

// Interface of HTTP request or stream (long-poll, chunked-response)
class STELLITE_EXPORT HttpClient {
 public:
  virtual ~HttpClient() {}

  // The delegate owned by HttpClient
  virtual int Request(const HttpRequest& request) = 0;

  // The delegate owned by HttpClient
  // if timeout set to zero, client cannot check resposne timeout
  // this rule apply a same rule on Stream() interface
  virtual int Request(const HttpRequest& request, int timeout) = 0;

  // append chunk context
  virtual bool AppendChunkToUpload(int request_id, const std::string& content,
                                   bool is_last_chunk) = 0;
};

} // namespace stellite

#endif // QUIC_INCLUDE_HTTP_CLIENT_H_
