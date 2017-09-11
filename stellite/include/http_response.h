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

#ifndef STELLITE_INCLUDE_HTTP_RESPONSE_H_
#define STELLITE_INCLUDE_HTTP_RESPONSE_H_

#include <string>
#include <memory>

#include "stellite_export.h"

namespace stellite {

// The proxy of net::HttpResponseHeaders
class STELLITE_EXPORT HttpResponseHeader {
 public:
  HttpResponseHeader();
  HttpResponseHeader(const HttpResponseHeader& other);
  HttpResponseHeader(const std::string& raw_header);
  virtual ~HttpResponseHeader();

  HttpResponseHeader& operator=(const HttpResponseHeader& other);

  // HTTP header enumerator that knows about http/1.1 header key and value
  bool EnumerateHeaderLines(size_t* iter,
                            std::string* name,
                            std::string* value) const;

  // HTTP header enumerator value that is related to a header key
  bool EnumerateHeader(size_t* iter,
                       const std::string& name,
                       std::string* value) const;

  // Check whether a header exists or not
  bool HasHeader(const std::string& name) const;

  // Return raw headers
  const std::string& raw_headers() const;

  void Reset(const std::string& raw_header);

private:
  class HttpResponseHeaderImpl;
  std::unique_ptr<HttpResponseHeaderImpl> impl_;
};

// The proxy of net::HttpResponseInfo
struct STELLITE_EXPORT HttpResponse {
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

  HttpResponse();
  HttpResponse(const HttpResponse& other);
  ~HttpResponse();

  HttpResponse& operator=(const HttpResponse& other);

  int response_code;
  long long content_length;

  std::string url;
  std::string status_text;
  std::string mime_type;
  std::string charset;
  std::string alpn_negotiated_protocol;

  bool network_accessed;
  bool server_data_unavailable;
  bool was_alpn_negotiated;
  bool was_cached;
  bool was_fetched_via_proxy;
  bool was_fetched_via_spdy;
  bool was_fetched_via_quic;

  uint64_t request_time;
  uint64_t response_time;

  ConnectionInfo connection_info;
  std::string connection_info_desc;

  HttpResponseHeader headers;
};

} // namespace stellite

#endif // QUIC_INCLUDE_HTTP_RESPONSE_H_
