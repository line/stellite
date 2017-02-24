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

#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "net/http/http_request_headers.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_status.h"
#include "stellite/fetcher/http_fetcher_impl.h"
#include "stellite/fetcher/http_fetcher_task.h"
#include "stellite/fetcher/http_request_context_getter.h"
#include "stellite/include/http_request.h"

namespace stellite {

HttpFetcher::HttpFetcher(
    scoped_refptr<HttpRequestContextGetter> context_getter)
    : http_request_context_getter_(context_getter),
      last_request_id_(0),
      network_task_runner_(context_getter->GetNetworkTaskRunner()),
      weak_factory_(this) {
}

HttpFetcher::~HttpFetcher() {}

int HttpFetcher::Request(const HttpRequest& http_request,
                         int64_t timeout,
                         base::WeakPtr<HttpFetcherTask::Visitor> d) {
  int request_id = ++last_request_id_;
  if (network_task_runner_->RunsTasksOnCurrentThread()) {
    StartRequest(request_id, http_request, timeout, d);
  } else {
    network_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&HttpFetcher::StartRequest,
                   weak_factory_.GetWeakPtr(),
                   request_id, http_request, timeout, d));
  }
  return request_id;

}

bool HttpFetcher::AppendChunkToUpload(int request_id,
                                      const std::string& data,
                                      bool is_last_chunk) {
  if (network_task_runner_->RunsTasksOnCurrentThread()) {
    StartAppendChunkToUpload(request_id, data, is_last_chunk);
  } else {
    network_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&HttpFetcher::StartAppendChunkToUpload,
                   weak_factory_.GetWeakPtr(),
                   request_id, data, is_last_chunk));
  }
  return true;
}

void HttpFetcher::StartRequest(int request_id,
                               const HttpRequest& http_request,
                               int64_t timeout,
                               base::WeakPtr<HttpFetcherTask::Visitor> d) {
  if (!GURL(http_request.url).is_valid()) {
    if (d.get()) {
      d->OnTaskError(request_id, nullptr, net::ERR_INVALID_URL);
    }
    return;
  }

  HttpFetcherTask* task = new HttpFetcherTask(this, request_id, d);
  task_map_.insert(std::make_pair(request_id, base::WrapUnique(task)));

  task->Start(http_request, timeout);
}

void HttpFetcher::StartAppendChunkToUpload(int request_id,
                                           const std::string& data,
                                           bool is_last_chunk) {
  TaskMap::iterator it = task_map_.find(request_id);
  if (it == task_map_.end()) {
    LOG(ERROR) << "invalid request_id for append chunk upload";
    return;
  }

  HttpFetcherTask* task = it->second.get();
  if (!data.size()) {
    HttpFetcherTask::Visitor* visitor = task->visitor();
    if (visitor) {
      visitor->OnTaskError(request_id, task->url_fetcher(),
                           net::ERR_INVALID_ARGUMENT);
    }

    return;
  }

  net::URLFetcher* source = task->url_fetcher();
  source->AppendChunkToUpload(data, is_last_chunk);
}

void HttpFetcher::Cancel(int request_id) {
  HttpFetcherTask* task = FindTask(request_id);
  if (task == nullptr) {
    return;
  }

  task->Stop();
}

void HttpFetcher::CancelAll() {
  while (!task_map_.empty()) {
    (task_map_.begin()->second.get())->Stop();
  }
}

HttpFetcherTask* HttpFetcher::FindTask(int request_id) {
  TaskMap::iterator it = task_map_.find(request_id);
  return it != task_map_.end() ? it->second.get() : nullptr;
}

void HttpFetcher::OnTaskComplete(int request_id) {
  task_map_.erase(request_id);
}

HttpRequestContextGetter* HttpFetcher::context_getter() {
  return http_request_context_getter_.get();
}

} // namespace stellite
