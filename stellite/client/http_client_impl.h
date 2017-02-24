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

#ifndef STELLITE_CLIENT_HTTP_CLIENT_IMPL_H_
#define STELLITE_CLIENT_HTTP_CLIENT_IMPL_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "stellite/fetcher/http_fetcher.h"
#include "stellite/include/http_client.h"
#include "stellite/include/stellite_export.h"
#include "stellite/fetcher/http_fetcher_task.h"

namespace stellite {
class HttpFetcher;
class HttpResponseDelegate;

class STELLITE_EXPORT HttpClientImpl : public HttpClient,
                                       public HttpFetcherTask::Visitor {
 public:
  HttpClientImpl(HttpFetcher* http_fetcher,
                 HttpResponseDelegate* response_delegate);
  ~HttpClientImpl() override;

  // implements HttpClient interface
  int Request(const HttpRequest& request) override;
  int Request(const HttpRequest& request, int timeout) override;

  // support chunked upload but if you want to this interface set
  // HttpRequest.chunked_upload a true
  bool AppendChunkToUpload(int request_id, const std::string& content,
                           bool is_last) override;

  // implements HttpFetcherTask::Visitor
  void OnTaskComplete(int request_id,
                      const net::URLFetcher* source,
                      const net::HttpResponseInfo* response_info) override;

  void OnTaskStream(int request_id,
                    const net::URLFetcher* source,
                    const net::HttpResponseInfo* response_info,
                    const char* data, size_t len, bool fin) override;

  void OnTaskError(int request_id,
                   const net::URLFetcher* source,
                   int error_code) override;

  void TearDown();

 private:
  using HttpResponseMap = std::map<int, std::unique_ptr<HttpResponse>>;

  HttpResponse* FindResponse(int request_id);
  HttpResponse* NewResponse(int request_id,
                            const net::URLFetcher* source,
                            const net::HttpResponseInfo* response_info);
  void ReleaseResponse(int request_id);

  HttpFetcher* http_fetcher_;
  HttpResponseDelegate* response_delegate_;

  // cache response data when streaming response or upload progress
  HttpResponseMap response_map_;

  // to keep net::HttpFetcherTask::Visitor's life-cycle scope
  base::WeakPtrFactory<HttpFetcherTask::Visitor> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(HttpClientImpl);
};

} // namespace stellite

#endif // STELLITE_CLIENT_HTTP_CLIENT_IMPL_H_
