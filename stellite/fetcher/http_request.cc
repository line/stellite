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

#include "stellite/include/http_request.h"

#include <memory>

#include "net/http/http_request_headers.h"

namespace stellite {

const char* kDefaultHttpRequestMethod = "GET";

class STELLITE_EXPORT HttpRequestHeader::HeaderImpl {
 public:
  HeaderImpl();
  HeaderImpl(net::HttpRequestHeaders* net_headers);
  virtual ~HeaderImpl();

  net::HttpRequestHeaders* headers() {
    return net_header_impl_.get();
  }

 private:
  std::unique_ptr<net::HttpRequestHeaders> net_header_impl_;
};

HttpRequestHeader::HeaderImpl::HeaderImpl()
    : net_header_impl_(new net::HttpRequestHeaders()) {
}

HttpRequestHeader::HeaderImpl::HeaderImpl(net::HttpRequestHeaders* net_headers)
    : net_header_impl_(new net::HttpRequestHeaders(*net_headers)) {
}

HttpRequestHeader::HeaderImpl::~HeaderImpl() {}

HttpRequestHeader::HttpRequestHeader()
    : header_impl_(new HeaderImpl()) {
}

HttpRequestHeader::HttpRequestHeader(const HttpRequestHeader& other)
    : header_impl_(new HeaderImpl(other.header_impl_->headers())) {
}

HttpRequestHeader::~HttpRequestHeader() {}

bool HttpRequestHeader::HasHeader(const std::string& key) const {
  DCHECK(header_impl_->headers());
  return header_impl_->headers()->HasHeader(key);
}

bool HttpRequestHeader::GetHeader(
    const std::string& key, std::string* value) const {
  DCHECK(header_impl_->headers());
  return header_impl_->headers()->GetHeader(key, value);
}

void HttpRequestHeader::SetHeader(const std::string& key,
                                  const std::string& value) {
  DCHECK(header_impl_->headers());
  return header_impl_->headers()->SetHeader(key, value);
}

void HttpRequestHeader::RemoveHeader(const std::string& key) {
  DCHECK(header_impl_->headers());
  header_impl_->headers()->RemoveHeader(key);
}

void HttpRequestHeader::SetRawHeader(const std::string& raw_header) {
  DCHECK(header_impl_->headers());
  header_impl_->headers()->AddHeadersFromString(raw_header);
}

void HttpRequestHeader::ClearHeader() {
  DCHECK(header_impl_->headers());
  header_impl_->headers()->Clear();
}

std::string HttpRequestHeader::ToString() const {
  DCHECK(header_impl_->headers());
  return header_impl_->headers()->ToString();
}

HttpRequest::HttpRequest()
    : url(),
      upload_stream(),
      request_type(GET),
      is_chunked_upload(false),
      is_stop_on_redirect(false),
      is_stream_response(false),
      max_retries_on_5xx(0),
      max_retries_on_network_change(0) {
}

HttpRequest::HttpRequest(const HttpRequest& other)
    : url(other.url),
      upload_stream(other.upload_stream.str()),
      request_type(other.request_type),
      is_chunked_upload(other.is_chunked_upload),
      is_stop_on_redirect(other.is_stop_on_redirect),
      is_stream_response(other.is_stream_response),
      max_retries_on_5xx(other.max_retries_on_5xx),
      max_retries_on_network_change(other.max_retries_on_network_change),
      headers(other.headers) {
}

HttpRequest::~HttpRequest() {}

} // namespace stellite
