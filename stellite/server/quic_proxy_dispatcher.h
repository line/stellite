// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// A server side dispatcher which dispatches a given client's data to their
// stream.

#ifndef STELLITE_SERVER_QUIC_PROXY_DISPATCHER_H_
#define STELLITE_SERVER_QUIC_PROXY_DISPATCHER_H_

#include <vector>
#include <memory>

#include "base/containers/hash_tables.h"
#include "base/macros.h"
#include "net/base/ip_endpoint.h"
#include "net/base/linked_hash_map.h"
#include "net/quic/core/quic_blocked_writer_interface.h"
#include "net/quic/core/quic_connection.h"
#include "net/quic/core/quic_protocol.h"
#include "net/tools/quic/quic_dispatcher.h"
#include "stellite/fetcher/http_request_context_getter.h"
#include "stellite/server/server_config.h"
#include "stellite/server/server_session.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace stellite {
class HttpFetcher;
}

namespace net {
class HttpRewrite;
class QuicConfig;
class QuicCryptoServerConfig;

// net::QuicProxyDispatcher inherits from net::QuicDispatcher.
// Stellite is necessary for proxy server to convert QUIC requests to
// TCP/HTTP requests. To do this, override
// net::QuicServerSessionBase::SendResponse to backend fetching
class NET_EXPORT QuicProxyDispatcher : public QuicDispatcher {
 public:
  QuicProxyDispatcher(
      const stellite::HttpRequestContextGetter::Params& fetcher_params,
      scoped_refptr<base::SingleThreadTaskRunner> fetcher_task_runner,
      const QuicConfig& quic_config,
      const QuicCryptoServerConfig* crypto_config,
      const ServerConfig& server_config,
      QuicVersionManager* version_manager,
      QuicConnectionHelperInterface* helper,
      QuicAlarmFactory* alarm_factory);

  ~QuicProxyDispatcher() override;

  const ServerConfig& server_config() { return server_config_; }

 protected:
  QuicServerSessionBase* CreateQuicSession(
      QuicConnectionId connection_id,
      const IPEndPoint& client_address) override;

  QuicPacketWriter* CreatePerConnectionWriter() override;

 private:
  const ServerConfig& server_config_;

  // Used by the helper_ to time alarms.
  QuicClock clock_;

  // To fetch HTTP requests on ProxyStream, URLContextGetter is required.
  scoped_refptr<stellite::HttpRequestContextGetter>
      http_request_context_getter_;

  std::unique_ptr<HttpRewrite> http_rewrite_;

  std::unique_ptr<stellite::HttpFetcher> http_fetcher_;

  DISALLOW_COPY_AND_ASSIGN(QuicProxyDispatcher);
};

}  // namespace net

#endif // #define STELLITE_SERVER_QUIC_PROXY_DISPATCHER_H_
