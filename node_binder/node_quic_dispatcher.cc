// Copyright 2017 LINE Corporation
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

#include "node_binder/node_quic_dispatcher.h"

#include "node_binder/node_quic_server_session.h"
#include "node_binder/node_quic_notify_interface.h"

namespace stellite {

NodeQuicDispatcher::NodeQuicDispatcher(
    const net::QuicConfig& config,
    const net::QuicCryptoServerConfig* crypto_config,
    net::QuicVersionManager* version_manager,
    std::unique_ptr<net::QuicConnectionHelperInterface> helper,
    std::unique_ptr<net::QuicCryptoServerStream::Helper> session_helper,
    std::unique_ptr<net::QuicAlarmFactory> alarm_factory,
    NodeQuicNotifyInterface* notify_interface)
    : QuicDispatcher(config,
                     crypto_config,
                     version_manager,
                     std::move(helper),
                     std::move(session_helper),
                     std::move(alarm_factory)),
      notify_interface_(notify_interface) {
  CHECK(notify_interface_);
}

NodeQuicDispatcher::~NodeQuicDispatcher() {}

net::QuicServerSessionBase* NodeQuicDispatcher::CreateQuicSession(
    net::QuicConnectionId connection_id,
    const net::IPEndPoint& client_address) {

  net::QuicConnection* connection = new net::QuicConnection(
      connection_id, client_address, helper(), alarm_factory(),
      CreatePerConnectionWriter(),
      /* own_writer= */ true, net::Perspective::IS_SERVER,
      GetSupportedVersions());

  NodeQuicServerSession* session =
      new NodeQuicServerSession(config(), connection, this, session_helper(),
                                crypto_config(), compressed_certs_cache(),
                                notify_interface_);
  session->Initialize();

  notify_interface_->OnSessionCreated(session);
  return session;
}

}  // namespace stellite
