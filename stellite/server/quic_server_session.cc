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

#include "stellite/server/quic_server_session.h"

#include "net/quic/core/quic_crypto_server_stream.h"

namespace net {

QuicServerSession::QuicServerSession(const QuicConfig& config,
                    QuicConnection* connection,
                    QuicServerSessionBase::Visitor* visitor,
                    QuicCryptoServerStream::Helper* helper,
                    const QuicCryptoServerConfig* crypto_config,
                    QuicCompressedCertsCache* compressed_certs_cache)
    : QuicServerSessionBase(config,
                            connection,
                            visitor,
                            helper,
                            crypto_config,
                            compressed_certs_cache) {
}

QuicServerSession::~QuicServerSession() {}

void QuicServerSession::OnStreamFrame(const QuicStreamFrame& frame) {
  if (!IsIncomingStream(frame.stream_id)) {
    connection()->CloseConnection(
        QUIC_INVALID_STREAM_ID, "client sent data on server push stream",
        ConnectionCloseBehavior::SEND_CONNECTION_CLOSE_PACKET);
    return;
  }
  QuicSpdySession::OnStreamFrame(frame);
}

QuicCryptoServerStreamBase* QuicServerSession::CreateQuicCryptoServerStream(
    const QuicCryptoServerConfig* crypto_config,
    QuicCompressedCertsCache* compressed_certs_cache) {
  return new QuicCryptoServerStream(
      crypto_config, compressed_certs_cache, true, this, stream_helper());
}

}  // namespace net
