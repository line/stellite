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
    bool enable_http2_alternative_service_with_different_host;
    bool enable_quic;
    bool enable_quic_alternative_service_with_different_host;
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
    bool using_memory_cache;
    int cache_max_size;
    int quic_idle_connection_timeout_seconds;
    int quic_max_server_configs_stored_in_properties;

    std::string accept_language;
    std::string proxy_host;
    std::string quic_user_agent_id;
    std::string user_agent;
    std::string http_disk_cache_path;

    std::vector<std::string> origins_to_force_quic_on;
  };

  HttpRequestContextGetter(
      Params context_params,
      scoped_refptr<base::SingleThreadTaskRunner> network_task_runner);

  // Implements net::URLRequestContextGetter
  net::URLRequestContext* GetURLRequestContext() override;

  scoped_refptr<base::SingleThreadTaskRunner> GetNetworkTaskRunner()
      const override;

  void set_host_resolver(std::unique_ptr<net::HostResolver> host_resolver);
  void set_cert_verifier(std::unique_ptr<net::CertVerifier> cert_verifier);

 private:
  ~HttpRequestContextGetter() override;

  bool BuildContext(Params params);

  Params context_params_;
  scoped_refptr<base::SingleThreadTaskRunner> network_task_runner_;

  base::FilePath transport_security_persister_path_;
  std::unique_ptr<net::HostResolver> host_resolver_;
  std::unique_ptr<net::ChannelIDService> channel_id_service_;
  std::unique_ptr<net::ProxyService> proxy_service_;
  std::unique_ptr<net::NetworkDelegate> network_delegate_;
  std::unique_ptr<net::ProxyDelegate> proxy_delegate_;
  std::unique_ptr<net::HttpAuthHandlerFactory> http_auth_handler_factory_;
  std::unique_ptr<net::CertVerifier> cert_verifier_;
  std::unique_ptr<net::CTVerifier> ct_verifier_;
  std::vector<std::unique_ptr<net::URLRequestInterceptor>>
      url_request_interceptors_;
  std::unique_ptr<net::HttpServerProperties> http_server_properties_;
  std::map<std::string,
      std::unique_ptr<net::URLRequestJobFactory::ProtocolHandler>>
          protocol_handlers_;

  std::unique_ptr<net::URLRequestContext> context_;

  DISALLOW_COPY_AND_ASSIGN(HttpRequestContextGetter);
};

} // namespace stellite

#endif // STELLITE_FETCHER_HTTP_REQUEST_CONTEXT_GETTER_H_
