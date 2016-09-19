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

#include "stellite/fetcher/http_fetcher_task.h"

#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "stellite/fetcher/http_fetcher.h"
#include "stellite/fetcher/http_fetcher_impl.h"
#include "stellite/fetcher/http_request_context_getter.h"
#include "stellite/logging/logging.h"

namespace net {

const char kDefaultContentType[] = "application/x-www-form-urlencoded";

HttpFetcherTask::HttpFetcherTask(HttpFetcher* http_fetcher,
                                 base::WeakPtr<HttpFetcherDelegate> delegate,
                                 int64_t timeout_msec)
    : http_fetcher_(http_fetcher),
      delegate_(delegate),
      timeout_(base::TimeDelta::FromMilliseconds(timeout_msec)) {
}

HttpFetcherTask::~HttpFetcherTask() {}

void HttpFetcherTask::Start(const GURL& url,
                            const URLFetcher::RequestType type,
                            const HttpRequestHeaders& headers,
                            const std::string& body) {
  // Set URL fetcher
  http_fetcher_impl_.reset(new HttpFetcherImpl(url, type, this));
  http_fetcher_impl_->SetRequestContext(http_fetcher_->context_getter());

  // Set body
  if (body.size()) {
    std::string content_type;
    if (!headers.GetHeader(HttpRequestHeaders::kContentType, &content_type)) {
      content_type = kDefaultContentType;
    }

    http_fetcher_impl_->SetUploadData(content_type, body);
  }

  // Set extra headers
  http_fetcher_impl_->SetExtraRequestHeaders(headers.ToString());

  // Set URL request context getter
  http_fetcher_impl_->SetStopOnRedirect(true);
  http_fetcher_impl_->Start();

  fetch_start_ = base::Time::Now();

  // Set timer
  url_fetch_timeout_timer_.reset(new base::OneShotTimer());
  url_fetch_timeout_timer_->Start(
      FROM_HERE,
      timeout_,
      this,
      &HttpFetcherTask::OnURLFetchTimeout);
}

void HttpFetcherTask::OnURLFetchComplete(const URLFetcher* source) {
  if (delegate_.get() == nullptr) {
    FLOG(ERROR) << "Fetch is complete but fetcher delegate has expired";
    return;
  }

  DCHECK(url_fetch_timeout_timer_.get());
  url_fetch_timeout_timer_->Stop();

  const URLRequestStatus& status = source->GetStatus();
  if (status.status() == URLRequestStatus::Status::FAILED) {
    FLOG(ERROR) << "Fetch has failed, error(" << status.error() << ") "
               << source->GetURL();
    FLOG(ERROR) << source->GetResponseCode();
  }

  base::TimeDelta duration = fetch_start_ - base::Time::Now();
  delegate_->OnFetchComplete(source, duration.InMillisecondsRoundedUp());
  http_fetcher_->OnTaskComplete(this);
}

void HttpFetcherTask::OnURLFetchTimeout() {
  if (delegate_.get() == nullptr) {
    return;
  }

  base::TimeDelta duration = fetch_start_ - base::Time::Now();
  delegate_->OnFetchTimeout(duration.InMillisecondsRoundedUp());
  http_fetcher_->OnTaskComplete(this);
}

} // namespace net
