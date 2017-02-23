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

#include "node_binder/node_quic_server_session.h"

#include "base/memory/ptr_util.h"
#include "net/quic/core/crypto/quic_compressed_certs_cache.h"
#include "net/quic/core/quic_connection.h"
#include "net/quic/core/quic_crypto_server_stream.h"
#include "node_binder/node_quic_notify_interface.h"
#include "node_binder/node_quic_server_stream.h"

namespace stellite {

NodeQuicServerSession::NodeQuicServerSession(
    const net::QuicConfig& quic_config,
    net::QuicConnection* quic_connection,
    QuicServerSessionBase::Visitor* visitor,
    net::QuicCryptoServerStream::Helper* helper,
    const net::QuicCryptoServerConfig* crypto_config,
    net::QuicCompressedCertsCache* compressed_certs_cache,
    NodeQuicNotifyInterface* node_notify)
    : QuicServerSessionBase(quic_config,
                            quic_connection,
                            visitor,
                            helper,
                            crypto_config,
                            compressed_certs_cache),
      node_notify_(node_notify) {
  CHECK(node_notify_);
}

NodeQuicServerSession::~NodeQuicServerSession() {
  delete connection();
}

void NodeQuicServerSession::OnConnectionClosed(
    net::QuicErrorCode error,
    const std::string& error_details,
    net::ConnectionCloseSource source) {
  QuicServerSessionBase::OnConnectionClosed(error, error_details, source);

  node_notify_->OnSessionClosed(this, error, error_details, source);
}

net::QuicCryptoServerStreamBase*
NodeQuicServerSession::CreateQuicCryptoServerStream(
    const net::QuicCryptoServerConfig* crypto_config,
    net::QuicCompressedCertsCache* compressed_certs_caches) {
  return new net::QuicCryptoServerStream(
      crypto_config, compressed_certs_caches,
      /* quic reloadable flag enable quic stateless reject support */ true,
      this, stream_helper());
}

net::QuicSpdyStream*
NodeQuicServerSession::CreateIncomingDynamicStream(net::QuicStreamId id) {
  if (!ShouldCreateIncomingDynamicStream(id)) {
    return nullptr;
  }

  NodeQuicServerStream* stream = new NodeQuicServerStream(id, this);
  ActivateStream(base::WrapUnique(stream));

  node_notify_->OnStreamCreated(this, stream);

  return stream;
}

net::QuicSpdyStream*
NodeQuicServerSession::CreateOutgoingDynamicStream(net::SpdyPriority priority) {
  if (!ShouldCreateOutgoingDynamicStream()) {
    return nullptr;
  }

  NodeQuicServerStream* stream =
      new NodeQuicServerStream(GetNextOutgoingStreamId(), this);
  ActivateStream(base::WrapUnique(stream));

  node_notify_->OnStreamCreated(this, stream);

  return stream;
}

}  // namespace stellite
