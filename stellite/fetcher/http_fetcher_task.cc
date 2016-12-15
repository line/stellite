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

namespace stellite {

const char kDefaultContentType[] = "application/x-www-form-urlencoded";

HttpFetcherTask::HttpFetcherTask(HttpFetcher* http_fetcher, int request_id,
                                 base::WeakPtr<Visitor> delegate)
    : http_fetcher_(http_fetcher),
      request_id_(request_id),
      visitor_(delegate) {
}

HttpFetcherTask::~HttpFetcherTask() {}

void HttpFetcherTask::Start(const HttpRequest& request,
                            int64_t timeout_msec) {
  GURL url(request.url);

  net::URLFetcher::RequestType request_type = net::URLFetcher::GET;
  if (request.request_type == HttpRequest::POST) {
    request_type = net::URLFetcher::POST;
  } else if (request.request_type == HttpRequest::HEAD) {
    request_type = net::URLFetcher::HEAD;
  } else if (request.request_type == HttpRequest::DELETE_REQUEST) {
    request_type = net::URLFetcher::DELETE_REQUEST;
  } else if (request.request_type == HttpRequest::PUT) {
    request_type = net::URLFetcher::PUT;
  } else if (request.request_type == HttpRequest::PATCH) {
    request_type = net::URLFetcher::PATCH;
  }

  // Set URL fetcher
  url_fetcher_.reset(
      new HttpFetcherImpl(url, request_type, this,
                          request.is_stream_response));

  net::HttpRequestHeaders headers;
  headers.AddHeadersFromString(request.headers.ToString());

  // set net::URLRequestContextGetter
  url_fetcher_->SetRequestContext(http_fetcher_->context_getter());

  // set payload
  std::string payload(request.upload_stream.str());
  DCHECK(!(payload.size() && request.is_chunked_upload));

  if (request.is_chunked_upload || payload.size()) {
    std::string content_type;
    if (!headers.GetHeader(net::HttpRequestHeaders::kContentType,
                           &content_type)) {
      content_type = kDefaultContentType;
    }

    if (request.is_chunked_upload) {
      url_fetcher_->SetChunkedUpload(content_type);
    } else {
      url_fetcher_->SetUploadData(content_type, payload);
    }
  }

  // Set extra headers
  url_fetcher_->SetExtraRequestHeaders(headers.ToString());

  // Set URL request context getter
  url_fetcher_->SetStopOnRedirect(request.is_stop_on_redirect);
  url_fetcher_->Start();

  if (timeout_msec > 0) {
    // Set timer
    url_fetch_timeout_timer_.reset(new base::OneShotTimer());
    url_fetch_timeout_timer_->Start(
        FROM_HERE,
        base::TimeDelta::FromMilliseconds(timeout_msec),
        this,
        &HttpFetcherTask::OnFetchTimeout);
  }
}

void HttpFetcherTask::Stop() {
  DCHECK(url_fetcher_.get());
  url_fetcher_->Stop();
}

void HttpFetcherTask::OnFetchComplete(
    const net::URLFetcher* source,
    const net::HttpResponseInfo* response_info) {

  if (url_fetch_timeout_timer_.get()) {
    url_fetch_timeout_timer_->Stop();
  }

  const net::URLRequestStatus& status = source->GetStatus();
  if (status.status() == net::URLRequestStatus::Status::FAILED) {
    LOG(ERROR) << "Fetch has failed, error(" << status.error() << ") "
        << source->GetURL();
    LOG(ERROR) << source->GetResponseCode();
  }

  if (visitor_.get()) {
    visitor_->OnTaskComplete(request_id_, source, response_info);
  }

  http_fetcher_->OnTaskComplete(request_id_);
}

void HttpFetcherTask::OnFetchStream(
    const net::URLFetcher* source,
    const net::HttpResponseInfo* response_info,
    const char* data, size_t len, bool fin) {

  if (visitor_.get()) {
    visitor_->OnTaskStream(request_id_, source, response_info,
                           data, len, fin);
  }

  if (fin) {
    http_fetcher_->OnTaskComplete(request_id_);
  }
}

void HttpFetcherTask::OnFetchTimeout() {
  if (visitor_.get()) {
    visitor_->OnTaskError(request_id_, url_fetcher(), net::ERR_TIMED_OUT);
  }

  http_fetcher_->OnTaskComplete(request_id_);
}

} // namespace net
