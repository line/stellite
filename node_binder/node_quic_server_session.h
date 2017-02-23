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

#ifndef NODE_BINDER_NODE_QUIC_SERVER_SESSION_H_
#define NODE_BINDER_NODE_QUIC_SERVER_SESSION_H_

#include "net/quic/core/quic_server_session_base.h"
#include "net/quic/core/quic_session.h"
#include "net/quic/core/quic_spdy_session.h"
#include "node/node.h"
#include "stellite/include/stellite_export.h"

namespace net {

class QuicCompressedCertsCache;
class QuicConnection;
class QuicCryptoServerConfig;

}  // namespace net

namespace stellite {

class NodeQuicNotifyInterface;
class NodeQuicVisitor;

class NodeQuicServerSession : public net::QuicServerSessionBase {
 public:
  NodeQuicServerSession(const net::QuicConfig& quic_config,
                        net::QuicConnection* quic_connection,
                        net::QuicServerSessionBase::Visitor* visitor,
                        net::QuicCryptoServerStream::Helper* helper,
                        const net::QuicCryptoServerConfig* crypto_config,
                        net::QuicCompressedCertsCache* compressed_certs_cache,
                        NodeQuicNotifyInterface* node_notify);
  ~NodeQuicServerSession() override;

  //  hook QuicConnectionDebugVisitor interface
  void OnConnectionClosed(net::QuicErrorCode error,
                          const std::string& error_details,
                          net::ConnectionCloseSource source) override;

 protected:
  // implements QuicServerSessionBase interface
  net::QuicCryptoServerStreamBase* CreateQuicCryptoServerStream(
      const net::QuicCryptoServerConfig* crypto_config,
      net::QuicCompressedCertsCache* compressed_certs_caches) override;

  // implements QuicSession interface
  net::QuicSpdyStream* CreateIncomingDynamicStream(
      net::QuicStreamId id) override;
  net::QuicSpdyStream* CreateOutgoingDynamicStream(
      net::SpdyPriority priority) override;

 private:
  NodeQuicNotifyInterface* node_notify_;

  DISALLOW_COPY_AND_ASSIGN(NodeQuicServerSession);
};

}  // namespace stellite

#endif  // NODE_BINDER_NODE_QUIC_SERVER_SESSION_H_
