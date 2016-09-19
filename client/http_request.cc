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

class STELLITE_EXPORT HttpRequestHeader::HttpRequestHeaderImpl {
 public:
  HttpRequestHeaderImpl()
      : impl_(new net::HttpRequestHeaders()) {
  }

  HttpRequestHeaderImpl(net::HttpRequestHeaders* net_headers)
      : impl_(new net::HttpRequestHeaders(*net_headers)) {
  }

  virtual ~HttpRequestHeaderImpl() {
  }

  net::HttpRequestHeaders* headers() {
    return impl_.get();
  }

 private:
  std::unique_ptr<net::HttpRequestHeaders> impl_;
};

HttpRequestHeader::HttpRequestHeader()
    : impl_(new HttpRequestHeaderImpl()) {
}

HttpRequestHeader::HttpRequestHeader(const HttpRequestHeader& other)
    : impl_(new HttpRequestHeaderImpl(other.impl_->headers())) {
}

HttpRequestHeader::~HttpRequestHeader() {}

bool HttpRequestHeader::HasHeader(const std::string& key) const {
  DCHECK(impl_->headers());
  return impl_->headers()->HasHeader(key);
}

bool HttpRequestHeader::GetHeader(
    const std::string& key, std::string* value) const {
  DCHECK(impl_->headers());
  return impl_->headers()->GetHeader(key, value);
}

void HttpRequestHeader::SetHeader(const std::string& key,
                                  const std::string& value) {
  DCHECK(impl_->headers());
  return impl_->headers()->SetHeader(key, value);
}

void HttpRequestHeader::RemoveHeader(const std::string& key) {
  DCHECK(impl_->headers());
  impl_->headers()->RemoveHeader(key);
}

void HttpRequestHeader::SetRawHeader(const std::string& raw_header) {
  DCHECK(impl_->headers());
  impl_->headers()->AddHeadersFromString(raw_header);
}

void HttpRequestHeader::ClearHeader() {
  DCHECK(impl_->headers());
  impl_->headers()->Clear();
}

std::string HttpRequestHeader::ToString() const {
  DCHECK(impl_->headers());
  return impl_->headers()->ToString();
}

HttpRequest::HttpRequest() {}

HttpRequest::~HttpRequest() {}

} // namespace stellite
