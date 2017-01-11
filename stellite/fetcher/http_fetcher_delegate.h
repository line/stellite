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

#ifndef STELLITE_FETCHER_HTTP_FETCHER_DELEGATE_H_
#define STELLITE_FETCHER_HTTP_FETCHER_DELEGATE_H_

#include <stddef.h>
#include <stdint.h>

#include "stellite/include/stellite_export.h"

namespace net {

class HttpResponseInfo;
class URLFetcher;

}  // namespace net

namespace stellite {

class STELLITE_EXPORT HttpFetcherDelegate {
 public:
  virtual ~HttpFetcherDelegate() {}

  virtual void OnFetchComplete(const net::URLFetcher* source,
                               const net::HttpResponseInfo* response_info) = 0;

  virtual void OnFetchStream(const net::URLFetcher* source,
                             const net::HttpResponseInfo* response_info,
                             const char* data, size_t len, bool fin) = 0;
};

}  // namespace stellite

#endif  // STELLITE_FETCHER_HTTP_FETCHER_DELEGATE_H_
