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

#ifndef STELLITE_SERVER_QUIC_PROXY_SESSION_H_
#define STELLITE_SERVER_QUIC_PROXY_SESSION_H_

#include "stellite/server/quic_server_session.h"
#include "url/gurl.h"

namespace stellite {
class HttpFetcher;
}  // namespace stellite

namespace net {

class NET_EXPORT QuicProxySession : public QuicServerSession {
 public:
  QuicProxySession(
      const QuicConfig& quic_config,
      QuicConnection* connection,
      QuicServerSessionBase::Visitor* visitor,
      QuicCryptoServerStream::Helper* helper,
      const QuicCryptoServerConfig* crypto_config,
      QuicCompressedCertsCache* compressed_certs_cache,
      stellite::HttpFetcher* http_fetcher,
      GURL proxy_pass);

  ~QuicProxySession() override;

 protected:
  QuicSpdyStream* CreateIncomingDynamicStream(QuicStreamId id) override;
  QuicSpdyStream* CreateOutgoingDynamicStream(SpdyPriority priority) override;

 private:
  stellite::HttpFetcher* proxy_fetcher_;
  GURL proxy_pass_;

  DISALLOW_COPY_AND_ASSIGN(QuicProxySession);
};

}  // namespace net

#endif  // STELLITE_SERVER_QUIC_PROXY_SESSION_H_
