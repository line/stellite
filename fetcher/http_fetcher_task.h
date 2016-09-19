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

#ifndef STELLITE_FETCHER_HTTP_FETCHER_TASK_H_
#define STELLITE_FETCHER_HTTP_FETCHER_TASK_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace base {
class Time;
}

namespace net {
class HttpFetcher;
class HttpFetcherDelegate;
class HttpRequestHeaders;
class URLRequestContextGetter;

class HttpFetcherTask : public URLFetcherDelegate {
 public:
  HttpFetcherTask(HttpFetcher* http_fetcher,
                  base::WeakPtr<HttpFetcherDelegate> delegate,
                  const int64_t timeout_msec);

  ~HttpFetcherTask() override;

  // Fetch URL request
  void Start(const GURL& url,
             const URLFetcher::RequestType type,
  const HttpRequestHeaders& headers,
  const std::string& body);

  // Implementation for net::URLFetcherDelegate
  void OnURLFetchComplete(const URLFetcher* source) override;

  virtual void OnURLFetchTimeout();

 private:
  HttpFetcher* http_fetcher_; /* not owned */
  base::WeakPtr<HttpFetcherDelegate> delegate_;
  std::unique_ptr<URLFetcher> http_fetcher_impl_;

  base::TimeDelta timeout_;
  base::Time fetch_start_;
  std::unique_ptr<base::OneShotTimer> url_fetch_timeout_timer_;

  DISALLOW_COPY_AND_ASSIGN(HttpFetcherTask);
};

} // namespace net

#endif // STELLITE_FETCHER_HTTP_FETCHER_TASK_H_
