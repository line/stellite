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

#include "stellite/server/server_session.h"

#include "base/memory/ptr_util.h"
#include "stellite/fetcher/http_fetcher.h"
#include "stellite/fetcher/http_rewrite.h"
#include "stellite/server/proxy_stream.h"
#include "stellite/stats/server_stats_recorder.h"

namespace net {

ServerSession::ServerSession(
    const QuicConfig& quic_config,
    const QuicCryptoServerConfig* crypto_config,
    const ServerConfig& server_config,
    QuicConnection* connection,
    QuicServerSessionBase::Visitor* visitor,
    QuicCryptoServerStream::Helper* helper,
    stellite::HttpFetcher* http_fetcher,
    const HttpRewrite* http_rewrite,
    QuicCompressedCertsCache* compressed_certs_cache)
    : QuicSimpleServerSession(quic_config,
                              connection,
                              visitor,
                              helper,
                              crypto_config,
                              compressed_certs_cache),
      server_config_(server_config),
      http_fetcher_(http_fetcher),
      http_rewrite_(http_rewrite),
      http_stats_recorder_(new ServerStatsRecorder()) {
}

ServerSession::~ServerSession() {
}

QuicSpdyStream* ServerSession::CreateIncomingDynamicStream(QuicStreamId id) {
  if (!ShouldCreateIncomingDynamicStream(id)) {
    return nullptr;
  }

  ProxyStream* stream = new ProxyStream(server_config_, id, this,
                                        http_fetcher_, http_rewrite_);
  ActivateStream(base::WrapUnique(stream));
  return stream;
}

QuicSimpleServerStream* ServerSession::CreateOutgoingDynamicStream(
    SpdyPriority priority) {
  if (!ShouldCreateOutgoingDynamicStream()) {
    return nullptr;
  }

  ProxyStream* stream = new ProxyStream(server_config_,
                                        GetNextOutgoingStreamId(),
                                        this,
                                        http_fetcher_,
                                        http_rewrite_);
  stream->SetPriority(priority);
  ActivateStream(base::WrapUnique(stream));
  return stream;
}

void ServerSession::AddHttpStat(const StatTag& tag, uint64_t value) {
  http_stats_recorder_->AddStat(tag, value);
}

HttpStats ServerSession::GetHttpStats() const {
  HttpStats stats;
  stats.http_sent = http_stats_recorder_->GetStat(kHttpSent);
  stats.http_timeout = http_stats_recorder_->GetStat(kHttpTimeout);
  stats.http_connection_failed = http_stats_recorder_->GetStat(kHttpFailed);
  stats.http_received = http_stats_recorder_->GetStat(kHttpReceived);
  return stats;
}

} // namespace net
