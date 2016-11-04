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

#include "stellite/include/http_response.h"

#include "net/http/http_response_headers.h"
#include "base/memory/ref_counted.h"
#include "base/logging.h"

namespace stellite {

class STELLITE_EXPORT HttpResponseHeader::HttpResponseHeaderImpl {
 public:
  HttpResponseHeaderImpl(const std::string& raw_headers)
      : headers_(new net::HttpResponseHeaders(raw_headers)) {
  }

  HttpResponseHeaderImpl(scoped_refptr<net::HttpResponseHeaders> headers)
      : headers_(headers) {
  }

  ~HttpResponseHeaderImpl() {}

  scoped_refptr<net::HttpResponseHeaders> headers() {
    return headers_.get();
  }

 private:
  scoped_refptr<net::HttpResponseHeaders> headers_;
};

HttpResponseHeader::HttpResponseHeader()
    : impl_(new HttpResponseHeaderImpl("")) {
}

HttpResponseHeader::HttpResponseHeader(const std::string& raw_headers)
    : impl_(new HttpResponseHeaderImpl(raw_headers)) {
}


HttpResponseHeader::HttpResponseHeader(const HttpResponseHeader& other)
    : impl_(new HttpResponseHeaderImpl(other.impl_->headers())) {
}

HttpResponseHeader::~HttpResponseHeader() {}

HttpResponseHeader& HttpResponseHeader::operator=(
    const HttpResponseHeader& other) {
  impl_.reset(new HttpResponseHeaderImpl(other.impl_->headers()));
  return *this;
}

bool HttpResponseHeader::EnumerateHeaderLines(size_t* iter,
                                              std::string* name,
                                              std::string* value) const {
  return impl_->headers()->EnumerateHeaderLines(iter, name, value);
}

bool HttpResponseHeader::EnumerateHeader(size_t* iter,
                                         const std::string& name,
                                         std::string* value) const {
  return impl_->headers()->EnumerateHeader(iter, name, value);
}

bool HttpResponseHeader::HasHeader(const std::string& name) const {
  return impl_->headers()->HasHeader(name);
}

const std::string& HttpResponseHeader::raw_headers() const {
  return impl_->headers()->raw_headers();
}

void HttpResponseHeader::Reset(const std::string& raw_header) {
  impl_.reset(new HttpResponseHeaderImpl(raw_header));
}

HttpResponse::HttpResponse()
    : headers("") {
}

HttpResponse::HttpResponse(const HttpResponse& other)
    : response_code(other.response_code),
      content_length(other.content_length),
      status_text(other.status_text),
      mime_type(other.mime_type),
      charset(other.charset),
      alpn_negotiated_protocol(other.alpn_negotiated_protocol),
      network_accessed(other.network_accessed),
      server_data_unavailable(other.server_data_unavailable),
      was_alpn_negotiated(other.was_alpn_negotiated),
      was_cached(other.was_cached),
      was_fetched_via_proxy(other.was_fetched_via_proxy),
      was_fetched_via_spdy(other.was_fetched_via_spdy),
      request_time(other.request_time),
      response_time(other.response_time),
      connection_info(other.connection_info),
      connection_info_desc(other.connection_info_desc),
      headers(other.headers) {
}

HttpResponse::~HttpResponse() {}

HttpResponse& HttpResponse::operator=(const HttpResponse& other) {
  response_code = other.response_code;
  content_length = other.content_length;
  status_text = other.status_text;
  mime_type = other.mime_type;
  charset = other.charset;
  alpn_negotiated_protocol = other.alpn_negotiated_protocol;
  network_accessed = other.network_accessed;
  server_data_unavailable = other.server_data_unavailable;
  was_alpn_negotiated = other.was_alpn_negotiated;
  was_cached = other.was_cached;
  was_fetched_via_proxy = other.was_fetched_via_proxy;
  was_fetched_via_spdy = other.was_fetched_via_spdy;
  request_time = other.request_time;
  response_time = other.response_time;
  connection_info = other.connection_info;
  connection_info_desc = other.connection_info_desc;
  headers = other.headers;
  return *this;
}

} // namespace stellite
