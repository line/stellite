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

#include "stellite/fetcher/http_fetcher.h"

#include "base/message_loop/message_loop.h"
#include "stellite/fetcher/http_fetcher_task.h"
#include "stellite/fetcher/http_request_context_getter.h"
#include "net/http/http_request_headers.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_status.h"

namespace net {

HttpFetcher::HttpFetcher(HttpRequestContextGetter* http_request_context_getter)
    : http_request_context_getter_(http_request_context_getter) {
}

HttpFetcher::~HttpFetcher() {}

void HttpFetcher::Fetch(const GURL& url,
                        const URLFetcher::RequestType type,
                        const HttpRequestHeaders& headers,
                        const std::string& body,
                        const int64_t timeout,
                        base::WeakPtr<HttpFetcherDelegate> delegate) {
  HttpFetcherTask* task = new HttpFetcherTask(this, delegate, timeout);
  task->Start(url, type, headers, body);
  task_cache_.insert(
      std::make_pair(task, std::unique_ptr<HttpFetcherTask>(task)));
}

void HttpFetcher::OnTaskComplete(const HttpFetcherTask* task) {
  task_cache_.erase(task);
}

HttpRequestContextGetter* HttpFetcher::context_getter() {
  return http_request_context_getter_;
}

} // namespace net
