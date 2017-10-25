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

#include "stellite/server/quic_proxy_session.h"

#include "base/memory/ptr_util.h"
#include "stellite/server/quic_proxy_stream.h"

namespace net {

QuicProxySession::QuicProxySession(
    const QuicConfig& quic_config,
    QuicConnection* connection,
    QuicServerSessionBase::Visitor* visitor,
    QuicCryptoServerStream::Helper* helper,
    const QuicCryptoServerConfig* crypto_config,
    QuicCompressedCertsCache* compressed_certs_cache,
    stellite::HttpFetcher* http_fetcher,
    GURL proxy_pass)
    : QuicServerSession(quic_config,
                        connection,
                        visitor,
                        helper,
                        crypto_config,
                        compressed_certs_cache),
      proxy_fetcher_(http_fetcher),
      proxy_pass_(proxy_pass) {
}

QuicProxySession::~QuicProxySession() {
  delete connection();
}

QuicSpdyStream* QuicProxySession::CreateIncomingDynamicStream(QuicStreamId id) {
  if (!ShouldCreateIncomingDynamicStream(id)) {
    return nullptr;
  }

  QuicSpdyStream* stream = new QuicProxyStream(id, this, proxy_fetcher_,
                                               proxy_pass_);
  ActivateStream(base::WrapUnique(stream));
  return stream;
}

QuicSpdyStream* QuicProxySession::CreateOutgoingDynamicStream(
    SpdyPriority priority) {
  if (!ShouldCreateOutgoingDynamicStream()) {
    return nullptr;
  }

  QuicSpdyStream* stream = new QuicProxyStream(GetNextOutgoingStreamId(),
                                               this, proxy_fetcher_,
                                               proxy_pass_);
  stream->SetPriority(priority);
  ActivateStream(base::WrapUnique(stream));
  return stream;
}

}  // namespace net
