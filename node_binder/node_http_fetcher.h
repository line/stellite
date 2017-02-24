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

#ifndef NODE_BINDER_NODE_HTTP_FETCHER_H_
#define NODE_BINDER_NODE_HTTP_FETCHER_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/non_thread_safe.h"
#include "stellite/fetcher/http_fetcher_task.h"
#include "stellite/fetcher/http_request_context_getter.h"
#include "stellite/include/stellite_export.h"

namespace stellite {

class HttpFetcher;

class NodeHttpFetcher : public base::NonThreadSafe {
 public:
  NodeHttpFetcher();
  ~NodeHttpFetcher();

  bool InitDefaultOptions();
  bool Init(const HttpRequestContextGetter::Params& params);

  int Request(const HttpRequest& request, int64_t timeout,
              base::WeakPtr<HttpFetcherTask::Visitor> visitor);

  bool AppendChunkToUpload(int request_id, const std::string& data, bool fin);

  void OnRequestComplete(int request_id);

 private:
  std::unique_ptr<HttpFetcher> impl_;

  DISALLOW_COPY_AND_ASSIGN(NodeHttpFetcher);
};

}  // namespace net

#endif  // NODE_BINDER_NODE_HTTP_FETCHER_H_
