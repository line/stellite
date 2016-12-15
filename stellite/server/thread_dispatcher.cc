// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stellite/server/thread_dispatcher.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "net/quic/chromium/quic_chromium_connection_helper.h"
#include "net/quic/core/crypto/quic_random.h"
#include "stellite/fetcher/http_fetcher.h"
#include "stellite/fetcher/http_request_context_getter.h"
#include "stellite/server/server_packet_writer.h"
#include "stellite/server/server_per_connection_packet_writer.h"
#include "stellite/server/server_session.h"
#include "stellite/server/server_session_helper.h"

namespace net {

class QuicServerSessionBase;

ThreadDispatcher::ThreadDispatcher(
    scoped_refptr<base::SingleThreadTaskRunner> fetcher_task_runner,
    const stellite::HttpRequestContextGetter::Params& fetcher_params,
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
          new stellite::HttpRequestContextGetter(fetcher_params,
                                                 fetcher_task_runner)),
      http_fetcher_(
          new stellite::HttpFetcher(http_request_context_getter_.get())) {
}

ThreadDispatcher::~ThreadDispatcher() {}

QuicServerSessionBase* ThreadDispatcher::CreateQuicSession(
    QuicConnectionId connection_id,
    const IPEndPoint& client_address) {

  QuicConnection* connection = new QuicConnection(
      connection_id, client_address, helper(), alarm_factory(),
      CreatePerConnectionWriter(),
      /* owns_writer */ true,
      Perspective::IS_SERVER, GetSupportedVersions());

  ServerSession* server_session =
      new ServerSession(config(), crypto_config(), server_config(),
                        connection, this, session_helper(),
                        http_fetcher_.get(), http_rewrite_.get(),
                        compressed_certs_cache());
  server_session->Initialize();

  return static_cast<QuicServerSessionBase*>(server_session);
}

QuicPacketWriter* ThreadDispatcher::CreatePerConnectionWriter() {
  return new ServerPerConnectionPacketWriter(
      static_cast<ServerPacketWriter*>(writer()));
}

}  // namespace net
