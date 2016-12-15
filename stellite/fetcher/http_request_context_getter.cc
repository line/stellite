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

#include "base/memory/ptr_util.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/multi_log_ct_verifier.h"
#include "net/proxy/proxy_config_service_fixed.h"
#include "net/proxy/proxy_service.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"
#include "stellite/client/http_ssl_config_service.h"

namespace stellite {

const char* kDefaultUserAgent = "stellite";

HttpRequestContextGetter::Params::Params()
    : enable_http2(true),
      enable_quic(true),
      ignore_certificate_errors(false),
      quic_close_sessions_on_ip_change(true),
      quic_delay_tcp_race(true),
      quic_disable_bidirectional_streams(false),
      quic_migrate_sessions_early(false),
      quic_migrate_sessions_on_network_change(false),
      quic_prefer_aes(false),
      quic_race_cert_verification(false),
      sdch_enable(false),
      throttling_enable(false),
      using_disk_cache(false),
      cache_max_size(0) {
}

HttpRequestContextGetter::Params::Params(const Params& other)
  : enable_http2(other.enable_http2),
    enable_quic(other.enable_quic),
    ignore_certificate_errors(other.ignore_certificate_errors),
    quic_close_sessions_on_ip_change(other.quic_close_sessions_on_ip_change),
    quic_delay_tcp_race(other.quic_delay_tcp_race),
    quic_disable_bidirectional_streams(
        other.quic_disable_bidirectional_streams),
    quic_migrate_sessions_early(other.quic_migrate_sessions_early),
    quic_migrate_sessions_on_network_change(
        other.quic_migrate_sessions_on_network_change),
    quic_prefer_aes(other.quic_prefer_aes),
    quic_race_cert_verification(other.quic_race_cert_verification),
    sdch_enable(other.sdch_enable),
    throttling_enable(other.throttling_enable),
    using_disk_cache(other.using_disk_cache),
    cache_max_size(other.cache_max_size),
    proxy_host(other.proxy_host),
    quic_host_whitelist(other.quic_host_whitelist) {
}

HttpRequestContextGetter::Params::~Params() {}

HttpRequestContextGetter::HttpRequestContextGetter(
    Params context_getter_params,
    scoped_refptr<base::SingleThreadTaskRunner> network_task_runner)
    : context_getter_params_(context_getter_params),
      network_task_runner_(network_task_runner) {
}

HttpRequestContextGetter::~HttpRequestContextGetter() {}

bool HttpRequestContextGetter::LazyInit(Params params) {
  DCHECK(network_task_runner_->BelongsToCurrentThread());

  if (url_request_context_.get()) {
    return false;
  }

  net::URLRequestContextBuilder builder;

  net::URLRequestContextBuilder::HttpCacheParams http_cache_params;

  // setting cache
  http_cache_params.type = params.using_disk_cache ?
      net::URLRequestContextBuilder::HttpCacheParams::DISK :
      net::URLRequestContextBuilder::HttpCacheParams::IN_MEMORY;
  http_cache_params.max_size = params.cache_max_size;
  builder.EnableHttpCache(http_cache_params);

  // setting http network session
  net::URLRequestContextBuilder::HttpNetworkSessionParams session_params;
  session_params.ignore_certificate_errors = params.ignore_certificate_errors;
  session_params.enable_http2 = params.enable_http2;

  // setting quic
  session_params.enable_quic = params.enable_quic;
  session_params.quic_close_sessions_on_ip_change =
      params.quic_close_sessions_on_ip_change;
  session_params.quic_migrate_sessions_on_network_change =
      params.quic_migrate_sessions_on_network_change;
  session_params.quic_migrate_sessions_early =
      params.quic_migrate_sessions_early;
  session_params.quic_disable_bidirectional_streams =
      params.quic_disable_bidirectional_streams;
  session_params.quic_race_cert_verification =
      params.quic_race_cert_verification;

  session_params.quic_host_whitelist.insert(
      params.quic_host_whitelist.begin(),
      params.quic_host_whitelist.end());

  builder.set_http_network_session_params(session_params);

  builder.set_sdch_enabled(params.sdch_enable);
  builder.set_throttling_enabled(params.throttling_enable);

  // setting host resolver
  net::HostResolver::Options resolver_options;
  builder.set_host_resolver(
      net::HostResolver::CreateSystemResolver(resolver_options, nullptr));

  // setting cert verifier
  builder.SetCertVerifier(net::CertVerifier::CreateDefault());

  // setting certificate transparenct verifier
  builder.set_ct_verifier(base::MakeUnique<net::MultiLogCTVerifier>());

  // setting proxy service
  builder.set_proxy_service(
      params.proxy_host.size() ?
      net::ProxyService::CreateFixed(params.proxy_host) :
      net::ProxyService::CreateDirect());

  // user agent
  builder.set_user_agent(kDefaultUserAgent);

  std::unique_ptr<net::URLRequestContext> context = builder.Build();

  // setting SSL config service
  context->set_ssl_config_service(new net::HttpSSLConfigService());

  url_request_context_ = std::move(context);

  return true;
}

net::URLRequestContext* HttpRequestContextGetter::GetURLRequestContext() {
  if (!url_request_context_.get()) {
    LazyInit(context_getter_params_);
  }

  return url_request_context_.get();
}

scoped_refptr<base::SingleThreadTaskRunner>
    HttpRequestContextGetter::GetNetworkTaskRunner() const {
  return network_task_runner_;
}

} // namespace net
