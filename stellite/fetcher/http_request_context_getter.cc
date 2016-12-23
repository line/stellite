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

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_util.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/cache_type.h"
#include "net/base/net_errors.h"
#include "net/base/network_delegate_impl.h"
#include "net/base/sdch_manager.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/ct_known_logs.h"
#include "net/cert/ct_log_verifier.h"
#include "net/cert/ct_policy_enforcer.h"
#include "net/cert/ct_verifier.h"
#include "net/cert/multi_log_ct_verifier.h"
#include "net/cookies/cookie_monster.h"
#include "net/dns/host_resolver.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_cache.h"
#include "net/http/http_network_layer.h"
#include "net/http/http_server_properties_impl.h"
#include "net/http/http_server_properties_manager.h"
#include "net/http/transport_security_persister.h"
#include "net/http/transport_security_state.h"
#include "net/proxy/proxy_config_service_fixed.h"
#include "net/proxy/proxy_service.h"
#include "net/quic/chromium/quic_stream_factory.h"
#include "net/ssl/channel_id_service.h"
#include "net/ssl/default_channel_id_store.h"
#include "net/ssl/ssl_config_service_defaults.h"
#include "net/url_request/data_protocol_handler.h"
#include "net/url_request/static_http_user_agent_settings.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_storage.h"
#include "net/url_request/url_request_intercepting_job_factory.h"
#include "net/url_request/url_request_interceptor.h"
#include "net/url_request/url_request_job_factory_impl.h"
#include "net/url_request/url_request_throttler_manager.h"
#include "stellite/client/http_ssl_config_service.h"

namespace stellite {

namespace {

class ContainerHttpRequestContext : public net::URLRequestContext {
 public:
  ContainerHttpRequestContext();
  ~ContainerHttpRequestContext() override;

  net::URLRequestContextStorage* storage() {
    return &storage_;
  }

 private:
  net::URLRequestContextStorage storage_;

  DISALLOW_COPY_AND_ASSIGN(ContainerHttpRequestContext);
};

ContainerHttpRequestContext::ContainerHttpRequestContext()
    : storage_(this) {
}

ContainerHttpRequestContext::~ContainerHttpRequestContext() {}

}  // namespace anonymous

HttpRequestContextGetter::Params::Params()
    : enable_http2(true),
      enable_http2_alternative_service_with_different_host(true),
      enable_quic(true),
      enable_quic_alternative_service_with_different_host(true),
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
      using_memory_cache(false),
      cache_max_size(0),
      quic_idle_connection_timeout_seconds(60),
      quic_max_server_configs_stored_in_properties(10) {
}

HttpRequestContextGetter::Params::Params(const Params& other)
  : enable_http2(other.enable_http2),
    enable_http2_alternative_service_with_different_host(
        other.enable_http2_alternative_service_with_different_host),
   enable_quic(other.enable_quic),
   enable_quic_alternative_service_with_different_host(
       other.enable_quic_alternative_service_with_different_host),
   ignore_certificate_errors(other.ignore_certificate_errors),
   quic_close_sessions_on_ip_change(other.quic_close_sessions_on_ip_change),
   quic_delay_tcp_race(other.quic_delay_tcp_race),
   quic_disable_bidirectional_streams(other.quic_disable_bidirectional_streams),
   quic_migrate_sessions_early(other.quic_migrate_sessions_early),
   quic_migrate_sessions_on_network_change(
       other.quic_migrate_sessions_on_network_change),
   quic_prefer_aes(other.quic_prefer_aes),
   quic_race_cert_verification(other.quic_race_cert_verification),
   sdch_enable(other.sdch_enable),
   throttling_enable(other.throttling_enable),
   using_disk_cache(other.using_disk_cache),
   using_memory_cache(other.using_memory_cache),
   cache_max_size(other.cache_max_size),
   quic_idle_connection_timeout_seconds(
       other.quic_idle_connection_timeout_seconds),
   quic_max_server_configs_stored_in_properties(
       other.quic_max_server_configs_stored_in_properties),
   accept_language(other.accept_language),
   proxy_host(other.proxy_host),
   quic_user_agent_id(other.quic_user_agent_id),
   user_agent(other.user_agent),
   http_disk_cache_path(other.http_disk_cache_path),
   origins_to_force_quic_on(other.origins_to_force_quic_on) {
}

HttpRequestContextGetter::Params::~Params() {}

HttpRequestContextGetter::HttpRequestContextGetter(
    Params context_params,
    scoped_refptr<base::SingleThreadTaskRunner> network_task_runner)
    : context_params_(context_params),
      network_task_runner_(network_task_runner) {
}

HttpRequestContextGetter::~HttpRequestContextGetter() {}

bool HttpRequestContextGetter::BuildContext(Params params) {
  DCHECK(network_task_runner_->BelongsToCurrentThread());

  if (context_.get()) {
    return false;
  }

  std::unique_ptr<ContainerHttpRequestContext> context(
      new ContainerHttpRequestContext());
  net::URLRequestContextStorage* storage = context->storage();

  storage->set_http_user_agent_settings(
      base::MakeUnique<net::StaticHttpUserAgentSettings>(params.accept_language,
                                                         params.user_agent));

  if (network_delegate_.get()) {
    storage->set_network_delegate(std::move(network_delegate_));
  } else {
    storage->set_network_delegate(base::MakeUnique<net::NetworkDelegateImpl>());
  }

  storage->set_net_log(base::WrapUnique(new net::NetLog));

  if (host_resolver_.get()) {
    storage->set_host_resolver(std::move(host_resolver_));
  } else {
    storage->set_host_resolver(
        net::HostResolver::CreateDefaultResolver(context->net_log()));
  }

  if (proxy_service_.get()) {
    storage->set_proxy_service(std::move(proxy_service_));
  } else {
    storage->set_proxy_service(
        net::ProxyService::CreateDirectWithNetLog(context->net_log()));
  }

  storage->set_ssl_config_service(new net::HttpSSLConfigService);

  if (http_auth_handler_factory_.get()) {
    storage->set_http_auth_handler_factory(
        std::move(http_auth_handler_factory_));
  } else {
    storage->set_http_auth_handler_factory(
        net::HttpAuthHandlerRegistryFactory::CreateDefault(
            context->host_resolver()));
  }

  std::unique_ptr<net::CookieStore> cookie_store(
      new net::CookieMonster(nullptr, nullptr));
  std::unique_ptr<net::ChannelIDService> channel_id_service(
      new net::ChannelIDService(new net::DefaultChannelIDStore(nullptr),
                                network_task_runner_));
  cookie_store->SetChannelIDServiceID(channel_id_service->GetUniqueID());
  storage->set_cookie_store(std::move(cookie_store));
  storage->set_channel_id_service(std::move(channel_id_service));

  if (params.sdch_enable) {
    storage->set_sdch_manager(base::MakeUnique<net::SdchManager>());
  }

  storage->set_transport_security_state(
      base::MakeUnique<net::TransportSecurityState>());

  if (http_server_properties_.get()) {
    storage->set_http_server_properties(std::move(http_server_properties_));
  } else {
    storage->set_http_server_properties(
        base::MakeUnique<net::HttpServerPropertiesImpl>());
  }

  if (cert_verifier_.get()) {
    storage->set_cert_verifier(std::move(cert_verifier_));
  } else {
    storage->set_cert_verifier(net::CertVerifier::CreateDefault());
  }

  if (ct_verifier_.get()) {
    storage->set_cert_transparency_verifier(std::move(ct_verifier_));
  } else {
    std::unique_ptr<net::MultiLogCTVerifier> ct_verifier =
        base::MakeUnique<net::MultiLogCTVerifier>();
    ct_verifier->AddLogs(net::ct::CreateLogVerifiersForKnownLogs());
    storage->set_cert_transparency_verifier(std::move(ct_verifier));
  }

  storage->set_ct_policy_enforcer(base::MakeUnique<net::CTPolicyEnforcer>());

  if (params.throttling_enable) {
    storage->set_throttler_manager(
        base::MakeUnique<net::URLRequestThrottlerManager>());
  }

  net::HttpNetworkSession::Params network_session_params;

  network_session_params.host_resolver = context->host_resolver();
  network_session_params.cert_verifier = context->cert_verifier();
  network_session_params.transport_security_state =
      context->transport_security_state();
  network_session_params.cert_transparency_verifier =
      context->cert_transparency_verifier();
  network_session_params.ct_policy_enforcer = context->ct_policy_enforcer();
  network_session_params.proxy_service = context->proxy_service();
  network_session_params.ssl_config_service = context->ssl_config_service();
  network_session_params.http_auth_handler_factory =
      context->http_auth_handler_factory();
  network_session_params.http_server_properties =
      context->http_server_properties();
  network_session_params.net_log = context->net_log();
  network_session_params.channel_id_service = context->channel_id_service();
  network_session_params.ignore_certificate_errors =
      context_params_.ignore_certificate_errors;
  network_session_params.enable_http2 =
      context_params_.enable_http2;
  network_session_params.enable_quic = context_params_.enable_quic;
  network_session_params.quic_max_server_configs_stored_in_properties =
      context_params_.quic_max_server_configs_stored_in_properties;
  network_session_params.quic_delay_tcp_race =
      context_params_.quic_delay_tcp_race;
  network_session_params.quic_idle_connection_timeout_seconds =
      context_params_.quic_idle_connection_timeout_seconds;
  network_session_params.quic_close_sessions_on_ip_change =
      context_params_.quic_close_sessions_on_ip_change;
  network_session_params.quic_migrate_sessions_on_network_change =
      context_params_.quic_migrate_sessions_on_network_change;
  network_session_params.quic_user_agent_id =
      context_params_.quic_user_agent_id;
  network_session_params.quic_prefer_aes =
      context_params_.quic_prefer_aes;
  network_session_params.quic_migrate_sessions_early =
      context_params_.quic_migrate_sessions_early;
  network_session_params.quic_disable_bidirectional_streams =
      context_params_.quic_disable_bidirectional_streams;
  network_session_params.quic_race_cert_verification =
      context_params_.quic_race_cert_verification;

  for (size_t i = 0; i < params.origins_to_force_quic_on.size(); ++i) {
    const std::string& hostname = params.origins_to_force_quic_on[i];
    network_session_params.origins_to_force_quic_on.insert(
        net::HostPortPair::FromString(hostname));
  }

  if (proxy_delegate_) {
    network_session_params.proxy_delegate = proxy_delegate_.get();
    storage->set_proxy_delegate(std::move(proxy_delegate_));
  }

  storage->set_http_network_session(
      base::MakeUnique<net::HttpNetworkSession>(network_session_params));

  if (params.using_disk_cache || params.using_memory_cache) {
    CHECK(params.http_disk_cache_path.size())
        << "http_disk_cache_path is empty error";

    std::unique_ptr<net::HttpCache::BackendFactory> cache_backend;
    if (params.using_disk_cache) {
      cache_backend.reset(new net::HttpCache::DefaultBackend(
              net::DISK_CACHE, net::CACHE_BACKEND_DEFAULT,
              base::FilePath(params.http_disk_cache_path),
              params.cache_max_size, network_task_runner_));
    } else {
      cache_backend = net::HttpCache::DefaultBackend::InMemory(
          params.cache_max_size);
    }
    storage->set_http_transaction_factory(
        base::MakeUnique<net::HttpCache>(storage->http_network_session(),
                                         std::move(cache_backend), true));
  } else {
    storage->set_http_transaction_factory(
        base::MakeUnique<net::HttpNetworkLayer>(
            storage->http_network_session()));
  }

  storage->set_job_factory(base::MakeUnique<net::URLRequestJobFactoryImpl>());

  context_ = std::move(context);
  return true;
}

net::URLRequestContext* HttpRequestContextGetter::GetURLRequestContext() {
  if (!context_.get()) {
    BuildContext(context_params_);
  }

  return context_.get();
}

scoped_refptr<base::SingleThreadTaskRunner>
    HttpRequestContextGetter::GetNetworkTaskRunner() const {
  return network_task_runner_;
}

void HttpRequestContextGetter::set_host_resolver(
    std::unique_ptr<net::HostResolver> host_resolver) {
  host_resolver_ = std::move(host_resolver);
}

void HttpRequestContextGetter::set_cert_verifier(
    std::unique_ptr<net::CertVerifier> cert_verifier) {
  cert_verifier_ = std::move(cert_verifier);
}

} // namespace net
