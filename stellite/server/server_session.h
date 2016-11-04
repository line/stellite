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

#ifndef STELLITE_SERVER_SERVER_SESSION_H_
#define STELLITE_SERVER_SERVER_SESSION_H_

#include <memory>

#include "stellite/server/server_config.h"
#include "stellite/stats/server_stats.h"
#include "net/tools/quic/quic_simple_server_session.h"

namespace net {
class HttpFetcher;
class HttpRewrite;
class QuicCompressedCertsCache;
class QuicConfig;
class QuicCryptoServerConfig;
class QuicSpdyStream;
class ServerConfig;
class ServerStatsRecorder;

class NET_EXPORT ServerSession : public QuicSimpleServerSession {
 public:
  ServerSession(
      const QuicConfig& quic_config,
      const QuicCryptoServerConfig* crypto_config,
      const ServerConfig& server_config,
      QuicConnection* connection,
      QuicServerSessionBase::Visitor* visitor,
      QuicCryptoServerStream::Helper* helper,
      HttpFetcher* http_fetcher,
      const HttpRewrite* http_rewrite,
      QuicCompressedCertsCache* compressed_certs_cache);
  ~ServerSession() override;

  void AddHttpStat(const StatTag& tag, uint64_t value);

  HttpStats GetHttpStats() const;

 protected:
  QuicSpdyStream* CreateIncomingDynamicStream(QuicStreamId id) override;

  QuicSimpleServerStream* CreateOutgoingDynamicStream(SpdyPriority priority)
      override;

 private:
  // Server configuration for HTTP reverse proxy
  const ServerConfig& server_config_;
  HttpFetcher* http_fetcher_; /* not owned */
  const HttpRewrite* http_rewrite_; /* not owned */
  std::unique_ptr<ServerStatsRecorder> http_stats_recorder_;

  DISALLOW_COPY_AND_ASSIGN(ServerSession);
};

} // namespace net

#endif // STELLITE_SERVER_SERVER_SESSION_H_
