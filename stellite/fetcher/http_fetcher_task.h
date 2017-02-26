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
#include "stellite/fetcher/http_fetcher_delegate.h"
#include "stellite/include/http_request.h"
#include "stellite/include/stellite_export.h"

namespace base {
class Time;
}

namespace stellite {
class HttpFetcher;
class HttpFetcherImpl;
class HttpRequestHeaders;
class URLRequestContextGetter;

class STELLITE_EXPORT HttpFetcherTask : public HttpFetcherDelegate {
 public:
  class Visitor {
   public:
    virtual ~Visitor() {}

    virtual void OnTaskComplete(int request_id,
                                const net::URLFetcher* source,
                                const net::HttpResponseInfo* response_info) = 0;

    virtual void OnTaskHeader(int request_id,
                              const net::URLFetcher* source,
                              const net::HttpResponseInfo* response_info) = 0;

    virtual void OnTaskStream(int request_id,
                              const char* data, size_t len, bool fin) = 0;

    virtual void OnTaskError(int request_id,
                             const net::URLFetcher* source,
                             int error_code) = 0;
  };

  HttpFetcherTask(HttpFetcher* http_fetcher, int request_id,
                  base::WeakPtr<Visitor> visitor);

  ~HttpFetcherTask() override;

  // Fetch URL request
  void Start(const HttpRequest& http_request, int64_t timeout_msec);
  void Stop();

  // Implementation for net::HttpFetcherDelegate
  void OnFetchComplete(const net::URLFetcher* source,
                       const net::HttpResponseInfo* response_info) override;

  void OnFetchHeader(const net::URLFetcher* source,
                     const net::HttpResponseInfo* response_info) override;

  void OnFetchStream(const char* data, size_t len, bool fin) override;

  void OnFetchTimeout();

  HttpFetcherImpl* url_fetcher() {
    return url_fetcher_.get();
  }

  Visitor* visitor() {
    return visitor_.get();
  }

 private:
  enum State {
    STATE_IDLE,
    STATE_STARTED,
    STATE_COMPLETE,
    STATE_CANCEL,
  };

  HttpFetcher* http_fetcher_; /* not owned */
  int request_id_;

  State state_;

  bool is_chunked_upload_;
  bool is_stream_response_;

  base::WeakPtr<Visitor> visitor_;
  std::unique_ptr<HttpFetcherImpl> url_fetcher_;

  base::TimeDelta timeout_;
  std::unique_ptr<base::OneShotTimer> url_fetch_timeout_timer_;

  DISALLOW_COPY_AND_ASSIGN(HttpFetcherTask);
};

} // namespace stellite

#endif // STELLITE_FETCHER_HTTP_FETCHER_TASK_H_
