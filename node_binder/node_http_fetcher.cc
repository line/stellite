// Copyright 2017 LINE Corporation
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

#include "node_binder/node_http_fetcher.h"

#include "base/threading/thread_task_runner_handle.h"
#include "stellite/fetcher/http_fetcher.h"

namespace stellite {

NodeHttpFetcher::NodeHttpFetcher() {}

NodeHttpFetcher::~NodeHttpFetcher() {}

bool NodeHttpFetcher::InitDefaultOptions() {
  DCHECK(CalledOnValidThread());

  stellite::HttpRequestContextGetter::Params params;
  // enable http2
  params.enable_http2 = true;
  params.enable_http2_alternative_service_with_different_host = true;

  // enable quic
  params.enable_quic = true;
  params.enable_quic_alternative_service_with_different_host = true;

  // allow certificate error or not
  params.ignore_certificate_errors = false;

  // disallow cache
  params.using_memory_cache = false;
  params.using_disk_cache = false;

  params.sdch_enable = true;

  return Init(params);
}

bool NodeHttpFetcher::Init(
    const stellite::HttpRequestContextGetter::Params& params) {
  DCHECK(CalledOnValidThread());

  if (impl_.get()) {
    return false;
  }

  stellite::HttpRequestContextGetter* context_getter =
      new stellite::HttpRequestContextGetter(
          params, base::ThreadTaskRunnerHandle::Get());

  impl_.reset(new stellite::HttpFetcher(context_getter));
  return true;
}

int NodeHttpFetcher::Request(const HttpRequest& request, int64_t timeout,
                             base::WeakPtr<HttpFetcherTask::Visitor> visitor) {
  DCHECK(CalledOnValidThread());
  DCHECK(impl_.get());
  return impl_->Request(request, timeout, visitor);
}

bool NodeHttpFetcher::AppendChunkToUpload(int request_id,
                                          const std::string& data, bool fin) {
  DCHECK(CalledOnValidThread());
  return impl_->AppendChunkToUpload(request_id, data, fin);
}

void NodeHttpFetcher::Cancel(int request_id) {
  DCHECK(CalledOnValidThread());
  impl_->Cancel(request_id);
}


void NodeHttpFetcher::OnRequestComplete(int request_id) {
  DCHECK(CalledOnValidThread());
  impl_->ReleaseRequest(request_id);
}

}  // namespace stellite
