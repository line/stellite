// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stellite/server/quic_proxy_dispatcher.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "net/quic/chromium/quic_chromium_connection_helper.h"
#include "net/quic/core/crypto/quic_random.h"
#include "stellite/fetcher/http_fetcher.h"
#include "stellite/fetcher/http_request_context_getter.h"
#include "stellite/server/quic_proxy_session.h"
#include "stellite/server/server_packet_writer.h"
#include "stellite/server/server_per_connection_packet_writer.h"
#include "stellite/server/server_session_helper.h"

namespace net {

class QuicServerSessionBase;

QuicProxyDispatcher::QuicProxyDispatcher(
    const stellite::HttpRequestContextGetter::Params& fetcher_params,
    scoped_refptr<base::SingleThreadTaskRunner> http_fetcher_task_runner,
    const QuicConfig& quic_config,
    const QuicCryptoServerConfig* crypto_config,
    const ServerConfig& server_config,
    QuicVersionManager* version_manager,
    QuicConnectionHelperInterface* helper,
    QuicAlarmFactory* alarm_factory)
    : QuicDispatcher(
        quic_config, crypto_config, version_manager,
        base::WrapUnique(helper),
        base::WrapUnique(new ServerSessionHelper(QuicRandom::GetInstance())),
        base::WrapUnique(alarm_factory)),
      server_config_(server_config),
      http_request_context_getter_(
          new stellite::HttpRequestContextGetter(
              fetcher_params, http_fetcher_task_runner)),
      http_fetcher_(
          new stellite::HttpFetcher(http_request_context_getter_.get())) {
}

QuicProxyDispatcher::~QuicProxyDispatcher() {}

QuicServerSessionBase* QuicProxyDispatcher::CreateQuicSession(
    QuicConnectionId connection_id,
    const IPEndPoint& client_address) {

  QuicConnection* connection = new QuicConnection(
      connection_id, client_address, helper(), alarm_factory(),
      CreatePerConnectionWriter(),
      /* owns_writer */ true,
      Perspective::IS_SERVER, GetSupportedVersions());

  QuicProxySession* session =
      new QuicProxySession(config(), connection, this, session_helper(),
                           crypto_config(), compressed_certs_cache(),
                           http_fetcher_.get(), server_config_.proxy_pass());
  session->Initialize();

  return static_cast<QuicServerSessionBase*>(session);
}

QuicPacketWriter* QuicProxyDispatcher::CreatePerConnectionWriter() {
  return new ServerPerConnectionPacketWriter(
      static_cast<ServerPacketWriter*>(writer()));
}

}  // namespace net
