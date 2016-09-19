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

#include "stellite/fetcher/http_request_context_getter.h"

#include "net/proxy/proxy_config_service_fixed.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"

namespace net {

HttpRequestContextGetter::HttpRequestContextGetter(
    scoped_refptr<base::SingleThreadTaskRunner> fetch_task_runner)
    : fetch_task_runner_(fetch_task_runner) {
}

HttpRequestContextGetter::~HttpRequestContextGetter() {}

net::URLRequestContext* HttpRequestContextGetter::GetURLRequestContext() {
  if (!url_request_context_.get()) {
    // Fixed proxy
    std::unique_ptr<net::ProxyConfigService> proxy_service(
        new net::ProxyConfigServiceFixed(net::ProxyConfig()));

    // Request context builder
    net::URLRequestContextBuilder builder;
    builder.DisableHttpCache();
    builder.set_proxy_config_service(std::move(proxy_service));

    std::unique_ptr<net::URLRequestContext> context = builder.Build();
    url_request_context_.reset(context.release());
  }

  return url_request_context_.get();
}

scoped_refptr<base::SingleThreadTaskRunner>
    HttpRequestContextGetter::GetNetworkTaskRunner() const {
  return fetch_task_runner_;
}

} // namespace net
