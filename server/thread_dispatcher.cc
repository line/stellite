// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stellite/server/thread_dispatcher.h"

#include <utility>

#include "stellite/fetcher/http_fetcher.h"
#include "stellite/fetcher/http_request_context_getter.h"
#include "stellite/server/server_packet_writer.h"
#include "stellite/server/server_per_connection_packet_writer.h"
#include "stellite/server/server_session.h"
#include "net/quic/crypto/quic_random.h"
#include "net/quic/quic_chromium_connection_helper.h"

namespace net {

class QuicServerSessionBase;

ThreadDispatcher::ThreadDispatcher(
    scoped_refptr<base::SingleThreadTaskRunner> fetch_task_runner,
    const QuicConfig& quic_config,
    const QuicCryptoServerConfig* crypto_config,
    const ServerConfig& server_config,
    const QuicVersionVector& supported_versions,
    QuicConnectionHelperInterface* helper,
    QuicAlarmFactory* alarm_factory)
    : QuicDispatcher(quic_config, crypto_config, supported_versions,
                     std::unique_ptr<QuicConnectionHelperInterface>(helper),
                     std::unique_ptr<QuicAlarmFactory>(alarm_factory)),
      server_config_(server_config),
      http_request_context_getter_(
          new HttpRequestContextGetter(fetch_task_runner)),
      http_fetcher_(new HttpFetcher(http_request_context_getter_.get())) {
}

ThreadDispatcher::~ThreadDispatcher() {}

QuicServerSessionBase* ThreadDispatcher::CreateQuicSession(
    QuicConnectionId connection_id,
    const IPEndPoint& client_address) {

  QuicConnection* connection = new QuicConnection(
      connection_id, client_address, helper(), alarm_factory(),
      CreatePerConnectionWriter(),
      /* owns_writer */ true,
      Perspective::IS_SERVER, supported_versions());

  ServerSession* server_session =
      new ServerSession(config(), crypto_config(), server_config(),
                        connection, this, http_fetcher_.get(),
                        http_rewrite_.get(),
                        compressed_certs_cache());
  server_session->Initialize();

  return static_cast<QuicServerSessionBase*>(server_session);
}

QuicPacketWriter* ThreadDispatcher::CreatePerConnectionWriter() {
  return new ServerPerConnectionPacketWriter(
      static_cast<ServerPacketWriter*>(writer()));
}

}  // namespace net
