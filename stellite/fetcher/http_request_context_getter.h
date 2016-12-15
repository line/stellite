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
#include "net/url_request/url_request_context_builder.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace net {
class URLRequestContext;
}

namespace stellite {

class HttpRequestContextGetter : public net::URLRequestContextGetter {
 public:
  struct Params {
    Params();
    Params(const Params& other);
    ~Params();

    bool enable_http2;
    bool enable_quic;
    bool ignore_certificate_errors;
    bool quic_close_sessions_on_ip_change;
    bool quic_delay_tcp_race;
    bool quic_disable_bidirectional_streams;
    bool quic_migrate_sessions_early;
    bool quic_migrate_sessions_on_network_change;
    bool quic_prefer_aes;
    bool quic_race_cert_verification;
    bool sdch_enable;
    bool throttling_enable;
    bool using_disk_cache;
    int cache_max_size;

    std::string proxy_host;
    std::vector<std::string> quic_host_whitelist;
  };

  HttpRequestContextGetter(
      Params context_getter_params,
      scoped_refptr<base::SingleThreadTaskRunner> network_task_runner);

  // Implements net::URLRequestContextGetter
  net::URLRequestContext* GetURLRequestContext() override;

  scoped_refptr<base::SingleThreadTaskRunner> GetNetworkTaskRunner()
      const override;

 private:
  ~HttpRequestContextGetter() override;

  bool LazyInit(Params params);

  Params context_getter_params_;

  std::unique_ptr<net::URLRequestContext> url_request_context_;

  scoped_refptr<base::SingleThreadTaskRunner> network_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(HttpRequestContextGetter);
};

} // namespace stellite

#endif // STELLITE_FETCHER_HTTP_REQUEST_CONTEXT_GETTER_H_
