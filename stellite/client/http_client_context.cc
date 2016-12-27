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

#include "stellite/include/http_client_context.h"

#include <memory>

#include "base/at_exit.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread.h"
#include "net/base/cache_type.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/ct_policy_enforcer.h"
#include "net/cert/multi_log_ct_verifier.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_server_properties_impl.h"
#include "net/log/net_log.h"
#include "net/proxy/proxy_service.h"
#include "net/socket/next_proto.h"
#include "net/ssl/channel_id_service.h"
#include "net/ssl/default_channel_id_store.h"
#include "net/ssl/ssl_config_service_defaults.h"
#include "stellite/client/backend_cache.h"
#include "stellite/client/backend_factory.h"
#include "stellite/client/cache_based_quic_server_info_factory.h"
#include "stellite/client/http_client_impl.h"
#include "stellite/client/http_ssl_config_service.h"
#include "stellite/client/network_transaction_client.h"
#include "stellite/client/network_transaction_factory.h"
#include "stellite/include/http_client.h"

#if defined(ANDROID)
#include "base/android/base_jni_registrar.h"
#include "base/android/context_utils.h"
#include "base/android/jni_android.h"
#include "base/android/jni_utils.h"
#include "net/android/net_jni_registrar.h"
#elif defined(USE_OPENSSL_CERTS)
#include "base/files/file_util.h"
#include "net/cert/cert_verify_proc_openssl.h"
#include "net/cert/test_root_certs.h"
#endif

// Default cache size
const int kDefaultCacheMaxSize = 1024 * 1024 * 32;

// QUIC socket receive buffer size
const int kQuicSocketReceiveBufferSize = 1024 * 1024;

namespace stellite {
HttpClientContext::Params::Params()
    : using_quic(true),
      using_spdy(true),
      using_http2(true),
      using_quic_disk_cache(false),
      ignore_certificate_errors(false),
      enable_http2_alternative_service_with_different_host(true),
      enable_quic_alternative_service_with_different_host(true) {
}

HttpClientContext::Params::Params(const Params& other) = default;

HttpClientContext::Params::~Params() {}

base::AtExitManager g_exit_manager;

class HttpClientContext::HttpClientContextImpl {
 public:
  HttpClientContextImpl(const Params& context_params)
      : on_init_completed_(base::WaitableEvent::ResetPolicy::MANUAL,
                           base::WaitableEvent::InitialState::NOT_SIGNALED),
        cache_path_(FILE_PATH_LITERAL(".http_client.cache")),
        context_params_(context_params) {
  }

  ~HttpClientContextImpl() {
    TearDown();
  }

  bool Initialize() {
    if (network_thread_.get()) {
      return false;
    }

    // Context thread
    network_thread_.reset(new base::Thread("network_thread"));
    base::Thread::Options thread_options;
    thread_options.message_loop_type = base::MessageLoop::TYPE_IO;

    network_thread_->StartWithOptions(thread_options);

  // For synchronized initialization
    on_init_completed_.Reset();

    network_thread_->task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&HttpClientContextImpl::InitOnNetworkThread,
                   base::Unretained(this)));

    on_init_completed_.Wait();

    return true;
  }

  bool TearDown() {
    if (!network_thread_.get() || !network_thread_->IsRunning()) {
      return false;
    }

    // Release context
    network_thread_->task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&HttpClientContextImpl::TearDownOnNetworkThread,
                   base::Unretained(this)));

    network_thread_->Stop();
    network_thread_.reset();

    return true;
  }

  bool Cancel() {
    if (!network_thread_.get() || !network_thread_->IsRunning()) {
      return false;
    }

    network_thread_->task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&HttpClientContextImpl::CancelOnNetworkThread,
                   base::Unretained(this)));
    return true;
  }

  HttpClient* CreateHttpClient(HttpResponseDelegate* visitor) {
    CHECK(network_thread_.get());

    net::NetworkTransactionClient* network_transaction_client =
        new net::NetworkTransactionClient(
            network_thread_->task_runner(),
            transaction_factory_.get());

    HttpClientImpl* client = new HttpClientImpl(network_transaction_client,
                                                visitor);
    http_client_map_.insert(
        http_client_map_.end(),
        std::make_pair(client, std::unique_ptr<HttpClientImpl>(client)));
    return client;
  }

  void ReleaseHttpClient(HttpClient* client) {
    network_thread_->task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&HttpClientContextImpl::ReleaseClientOnNetworkThread,
                   base::Unretained(this), client));
  }

 private:
  void ReleaseClientOnNetworkThread(HttpClient* client) {
    CHECK(client);

    HttpClientImpl* client_impl = static_cast<HttpClientImpl*>(client);
    client_impl->TearDown();
    http_client_map_.erase(client_impl);
  }

  void InitOnNetworkThread() {
    CHECK(base::MessageLoop::current() == network_thread_->message_loop());

    // HTTP server properties
    http_server_properties_.reset(new net::HttpServerPropertiesImpl());

    // Initialize network session
    net::HttpNetworkSession::Params params;

    // Enable NPN, ALPN is always supported
    params.enable_tcp_fast_open_for_ssl = true;

    params.ignore_certificate_errors =
        context_params_.ignore_certificate_errors;
    params.enable_http2_alternative_service_with_different_host =
        context_params_.enable_http2_alternative_service_with_different_host;
    params.enable_quic_alternative_service_with_different_host =
        context_params_.enable_quic_alternative_service_with_different_host;

    // Support QUIC
    if (context_params_.using_quic) {
      params.enable_quic = true;
      params.enable_quic_port_selection = true;
      params.quic_clock = nullptr;
      params.quic_disable_connection_pooling = false;
      params.quic_enable_connection_racing = true;
      params.quic_enable_non_blocking_io = true;
      params.quic_load_server_info_timeout_srtt_multiplier = 0.25f;
      params.quic_random = nullptr;
      params.quic_socket_receive_buffer_size = kQuicSocketReceiveBufferSize;

      // Using quic_server config
      params.quic_disable_disk_cache = !context_params_.using_quic_disk_cache;

      // Using connection migration
      params.quic_migrate_sessions_on_network_change = true;
      params.quic_migrate_sessions_early = true;

      // Using QUIC without QUIC Discovery
      if (context_params_.origin_to_force_quic_on.size()) {
        params.origins_to_force_quic_on.insert(
            params.origins_to_force_quic_on.end(),
            net::HostPortPair::FromString(
                context_params_.origin_to_force_quic_on));
      }
    }

    // Support SPDY
    if (context_params_.using_spdy) {
      params.enable_spdy_ping_based_connection_checking = true;
    }

    // Support HTTP2
    if (context_params_.using_http2) {
      params.enable_http2 = true;
    }

    // Channel ID service
    channel_id_service_.reset(new net::ChannelIDService(
        new net::DefaultChannelIDStore(nullptr),
        base::MessageLoop::current()->task_runner()));
    params.channel_id_service = channel_id_service_.get();

    // Initialize resolver
    net::HostResolver::Options resolve_option;
    host_resolver_ = net::HostResolver::CreateSystemResolver(resolve_option,
                                                             &context_netlog_);
    params.host_resolver = host_resolver_.get();

    // Initialize certification verifier
    cert_verifier_ = net::CertVerifier::CreateDefault();
    params.cert_verifier = cert_verifier_.get();

    // Initialize transport security state
    transport_security_state_.reset(new net::TransportSecurityState());
    params.transport_security_state = transport_security_state_.get();

    ct_verifier_.reset(new net::MultiLogCTVerifier());
    params.cert_transparency_verifier = ct_verifier_.get();

    ct_policy_enforcer_.reset(new net::CTPolicyEnforcer());
    params.ct_policy_enforcer = ct_policy_enforcer_.get();

    // Initialize proxy service
    proxy_service_ =
        context_params_.proxy_host.size() ?
        net::ProxyService::CreateFixed(context_params_.proxy_host) :
        net::ProxyService::CreateDirect();
    params.proxy_service = proxy_service_.get();

    // SSL config service
    ssl_config_service_ = new net::HttpSSLConfigService();
    params.ssl_config_service = ssl_config_service_.get();

    // Initialize auth handler factory
    auth_handler_factory_ =
        net::HttpAuthHandlerFactory::CreateDefault(host_resolver_.get());
    params.http_auth_handler_factory = auth_handler_factory_.get();

    params.http_server_properties = http_server_properties_.get();

    // Build transaction factory
    net::HttpNetworkSession* session = new net::HttpNetworkSession(params);
    transaction_factory_.reset(new net::NetworkTransactionFactory(session));

    if (context_params_.using_quic) {
      if (context_params_.using_quic_disk_cache) {
        // Set disk cache
        backend_factory_.reset(
            new net::BackendFactory(net::DISK_CACHE,
                                    net::CACHE_BACKEND_DEFAULT,
                                    cache_path_,
                                    kDefaultCacheMaxSize,
                                    network_thread_->task_runner()));
      } else {
        backend_factory_.reset(
            net::BackendFactory::InMemory(kDefaultCacheMaxSize));
      }

      backend_cache_.reset(new net::BackendCache(&context_netlog_,
                                                 backend_factory_.get()));

      net::QuicStreamFactory* factory = session->quic_stream_factory();
      if (factory) {
        // Setting a QUIC server info factory
        net::CacheBasedQuicServerInfoFactory* quic_server_info_factory =
            new net::CacheBasedQuicServerInfoFactory(backend_cache_.get());
        factory->set_quic_server_info_factory(quic_server_info_factory);
      }
    }

    on_init_completed_.Signal();
  }

  void TearDownOnNetworkThread() {
    CHECK(base::MessageLoop::current() == network_thread_->message_loop());

    // Release all the connected transactions
    transaction_factory_.reset();

    // Release all the clients
    http_client_map_.clear();

    // Remove backend cache
    backend_cache_.reset();
    backend_factory_.reset();

    // Release something ...
    auth_handler_factory_.reset();
    cert_verifier_.reset();
    channel_id_service_.reset();
    ct_policy_enforcer_.reset();
    ct_verifier_.reset();
    host_resolver_.reset();
    http_server_properties_.reset();
    proxy_service_.reset();
    ssl_config_service_ = nullptr;
    transport_security_state_.reset();

    base::RunLoop().RunUntilIdle();
  }

  void CancelOnNetworkThread() {
    CHECK(base::MessageLoop::current() == network_thread_->message_loop());
    if (!transaction_factory_.get()) {
      return;
    }
    transaction_factory_->OnCancel();
  }

  // TODO(@snibug): Check Android's Stellite AtExitManager
  static base::AtExitManager g_at_exit_manager;

  typedef std::map<HttpClientImpl*, std::unique_ptr<HttpClientImpl>>
      HttpClientImplMap;
  typedef std::map<HttpClientImpl*, HttpResponseDelegate*>
      HttpResponseDelegateMap;
  typedef std::set<std::unique_ptr<HttpClientImpl>> HttpClientImplContainer;

  base::WaitableEvent on_init_completed_;

  std::unique_ptr<base::Thread> network_thread_;
  std::unique_ptr<net::CertVerifier> cert_verifier_;
  std::unique_ptr<net::HostResolver> host_resolver_;
  std::unique_ptr<net::TransportSecurityState> transport_security_state_;
  std::unique_ptr<net::CTVerifier> ct_verifier_;
  std::unique_ptr<net::CTPolicyEnforcer> ct_policy_enforcer_;
  std::unique_ptr<net::HttpAuthHandlerFactory> auth_handler_factory_;
  std::unique_ptr<net::HttpServerPropertiesImpl> http_server_properties_;
  std::unique_ptr<net::NetworkTransactionFactory> transaction_factory_;
  std::unique_ptr<net::ProxyService> proxy_service_;
  scoped_refptr<net::SSLConfigService> ssl_config_service_;

  // Cache
  base::FilePath cache_path_;
  std::unique_ptr<net::BackendFactory> backend_factory_;
  std::unique_ptr<net::BackendCache> backend_cache_;
  std::unique_ptr<net::CacheBasedQuicServerInfoFactory>
      quic_server_info_factory_;

  std::unique_ptr<net::ChannelIDService> channel_id_service_;

  const Params context_params_;

  HttpClientImplMap http_client_map_;

  net::NetLog context_netlog_;

  DISALLOW_COPY_AND_ASSIGN(HttpClientContextImpl);
};

HttpClientContext::HttpClientContext(const Params& context_params)
  : impl_(new HttpClientContext::HttpClientContextImpl(context_params)) {
}

HttpClientContext::~HttpClientContext() {
  TearDown();

  if (impl_) {
    delete impl_;
    impl_ = NULL;
  }
}

bool HttpClientContext::Initialize() {
  return impl_->Initialize();
}

bool HttpClientContext::TearDown() {
  // Release context implementation
  return impl_->TearDown();
}

bool HttpClientContext::Cancel() {
  return impl_->Cancel();
}

HttpClient* HttpClientContext::CreateHttpClient(HttpResponseDelegate* visitor) {
  return impl_->CreateHttpClient(visitor);
}

void HttpClientContext::ReleaseHttpClient(stellite::HttpClient* client) {
  CHECK(client);
  impl_->ReleaseHttpClient(client);
}

#if defined(ANDROID)

// Static
bool HttpClientContext::InitVM(JavaVM* vm) {
  if (base::android::IsVMInitialized()) {
    return false;
  }

  // Initialize
  base::android::InitVM(vm);

  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::InitReplacementClassLoader(
      env, base::android::GetClassLoader(env));

  if (!base::android::RegisterJni(env)) {
    return false;
  }

  if (!net::android::RegisterJni(env)) {
    return false;
  }

  return true;
}

#elif defined(USE_OPENSSL_CERTS)

bool HttpClientContext::ResetCertBundle(const std::string& filename) {
  std::string raw_cert;
  if (!base::ReadFileToString(base::FilePath(filename), &raw_cert)) {
    LOG(ERROR) << "cannot load certificate";
    return false;
  }

  net::CertificateList certs =
      net::X509Certificate::CreateCertificateListFromBytes(
          raw_cert.data(),
          raw_cert.length(),
          net::X509Certificate::FORMAT_AUTO);

  net::TestRootCerts* root_certs = net::TestRootCerts::GetInstance();
  for (net::CertificateList::const_iterator it = certs.begin();
       it != certs.end(); ++it) {
    if (!root_certs->Add(it->get())) {
      return false;
    }
  }

  return !root_certs->IsEmpty();
}

#endif

} // namespace stellite
