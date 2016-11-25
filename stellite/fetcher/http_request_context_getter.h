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

#ifndef STELLITE_FETCHER_HTTP_REQUEST_CONTEXT_GETTER_H_
#define STELLITE_FETCHER_HTTP_REQUEST_CONTEXT_GETTER_H_

#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "net/url_request/url_request_context_getter.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace net {
class URLRequestContext;

class HttpRequestContextGetter : public net::URLRequestContextGetter {
 public:
  HttpRequestContextGetter(
      scoped_refptr<base::SingleThreadTaskRunner> fetch_task_runner);

  // Implements net::URLRequestContextGetter
  net::URLRequestContext* GetURLRequestContext() override;

  scoped_refptr<base::SingleThreadTaskRunner> GetNetworkTaskRunner()
      const override;

 private:
  ~HttpRequestContextGetter() override;

  std::unique_ptr<net::URLRequestContext> url_request_context_;

  scoped_refptr<base::SingleThreadTaskRunner> fetch_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(HttpRequestContextGetter);
};

} // namespace net

#endif // STELLITE_FETCHER_HTTP_REQUEST_CONTEXT_GETTER_H_
