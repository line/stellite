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

#ifndef STELLITE_FETCHER_HTTP_FETCHER_H_
#define STELLITE_FETCHER_HTTP_FETCHER_H_

#include <map>
#include <memory>

#include "base/memory/weak_ptr.h"
#include "net/spdy/spdy_header_block.h"
#include "net/spdy/spdy_protocol.h"
#include "net/url_request/url_fetcher.h"
#include "stellite/fetcher/http_fetcher_task.h"
#include "stellite/include/http_request.h"
#include "stellite/include/stellite_export.h"

namespace base {

class SingleThreadTaskRunner;

}  // namespace base

namespace stellite {

class HttpFetcherDelegate;
class HttpRequestContextGetter;

class STELLITE_EXPORT HttpFetcher {
 public:
  HttpFetcher(scoped_refptr<HttpRequestContextGetter> context_getter);
  ~HttpFetcher();

  int Request(const HttpRequest& request, int64_t timeout,
              base::WeakPtr<HttpFetcherTask::Visitor> delegate);
  bool AppendChunkToUpload(int request_id, const std::string& data, bool fin);
  void Cancel(int request_id);

  void CancelAll();

  // release task when it was done
  void ReleaseRequest(int request_id);

  HttpRequestContextGetter* context_getter() { return context_getter_.get(); }

 private:
  typedef std::map<int, std::unique_ptr<HttpFetcherTask>> RequestMap;

  HttpFetcherTask* FindTask(int request_id);

  // start request that work on base::SingleThreadTaskRunner
  void StartRequest(int request_id, const HttpRequest& http_request,
                    int64_t timeout,
                    base::WeakPtr<HttpFetcherTask::Visitor> delegate);
  void StartAppendChunkToUpload(int request_id, const std::string& data,
                                bool fin);
  void StartCancel(int request_id);
  void StartRelease(int request_id);

  // select TaskRunner between current thread or network thread
  base::SingleThreadTaskRunner* GetTaskRunner();

  int last_request_id_;
  RequestMap request_map_; // task container

  scoped_refptr<HttpRequestContextGetter> context_getter_;

  base::WeakPtrFactory<HttpFetcher> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(HttpFetcher);
};

} // namespace net

#endif  // STELLITE_FETCHER_HTTP_FETCHER_H_
