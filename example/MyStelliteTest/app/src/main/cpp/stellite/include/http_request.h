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

#ifndef STELLITE_INCLUDE_HTTP_REQUEST_H_
#define STELLITE_INCLUDE_HTTP_REQUEST_H_

#include <memory>
#include <sstream>
#include <string>

#include "stellite_export.h"

namespace stellite {

// The proxy of net::HttpRequestHeaders
class STELLITE_EXPORT HttpRequestHeader {
 public:
  explicit HttpRequestHeader();
  HttpRequestHeader(const HttpRequestHeader& other);
  ~HttpRequestHeader();

  void SetRawHeader(const std::string& raw_header);

  // HTTP header modifier
  bool HasHeader(const std::string& key) const;
  bool GetHeader(const std::string& key, std::string* value) const;
  void SetHeader(const std::string& key, const std::string& value);
  void RemoveHeader(const std::string& key);
  void ClearHeader();

  std::string ToString() const;

 private:
  class HeaderImpl;
  std::unique_ptr<HeaderImpl> header_impl_;
};

// HttpRequest consisting of request URL, method, payload, HTTP headers
struct STELLITE_EXPORT HttpRequest {
  enum RequestType : int {
    GET,
    POST,
    HEAD,
    DELETE_REQUEST,   // DELETE is already taken on Windows.
    PUT,
    PATCH,
  };

  explicit HttpRequest();
  HttpRequest(const HttpRequest& other);
  ~HttpRequest();

  std::string url;
  std::stringstream upload_stream;

  RequestType request_type;

  bool is_chunked_upload;
  bool is_stop_on_redirect;
  bool is_stream_response;
  int max_retries_on_5xx;
  int max_retries_on_network_change;

  HttpRequestHeader headers;
};

} // namespace stellite

#endif // STELLITE_INCLUDE_HTTP_REQUEST_H_
