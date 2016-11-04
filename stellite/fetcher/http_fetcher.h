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

#ifndef QUIC_FETCHER_HTTP_FETCHER_H_
#define QUIC_FETCHER_HTTP_FETCHER_H_

#include <map>
#include <memory>

#include "base/memory/weak_ptr.h"
#include "net/spdy/spdy_header_block.h"
#include "net/spdy/spdy_protocol.h"
#include "net/url_request/url_fetcher.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace net {
class HttpFetcherTask;
class HttpRequestContextGetter;

class HttpFetcherDelegate {
 public:
  virtual ~HttpFetcherDelegate() {}
  virtual void OnFetchComplete(const URLFetcher* source, int64_t msec) = 0;
  virtual void OnFetchTimeout(int64_t msec) = 0;
};

class HttpFetcher {
 public:
  HttpFetcher(HttpRequestContextGetter* http_request_context_getter);
  ~HttpFetcher();

  void Fetch(const GURL& url,
             const URLFetcher::RequestType type,
             const HttpRequestHeaders& headers,
             const std::string& body,
             const int64_t timeout,
             base::WeakPtr<HttpFetcherDelegate> delegate);

  void OnTaskComplete(const HttpFetcherTask* task);

  HttpRequestContextGetter* context_getter();

 private:
  typedef std::map<const HttpFetcherTask*,
                   std::unique_ptr<HttpFetcherTask>> TaskMap;

  TaskMap task_cache_; // task container
  HttpRequestContextGetter* http_request_context_getter_; /* not owned */

  DISALLOW_COPY_AND_ASSIGN(HttpFetcher);
};

} // namespace net

#endif
