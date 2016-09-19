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

#include "stellite/server/quic_thread_server.h"

#include "base/threading/thread.h"
#include "stellite/fetcher/http_rewrite.h"
#include "stellite/logging/logging.h"
#include "stellite/server/thread_dispatcher.h"
#include "stellite/server/thread_worker.h"
#include "net/base/net_errors.h"
#include "net/quic/crypto/quic_random.h"
#include "net/tools/quic/quic_dispatcher.h"

namespace net {
class ServerPacketWriter;

const char* kSourceAddressTokenSecret = "secret";

QuicThreadServer::QuicThreadServer(const QuicConfig& quic_config,
                                   const ServerConfig& server_config,
                                   const QuicVersionVector& supported_versions,
                                   ProofSource* proof_source)
    : quic_config_(quic_config),
      crypto_config_(kSourceAddressTokenSecret,
                     QuicRandom::GetInstance(),
                     proof_source),
      server_config_(server_config),
      supported_versions_(supported_versions) {
}

QuicThreadServer::~QuicThreadServer() {
}

bool QuicThreadServer::Start(size_t worker_size,
                             const IPEndPoint& quic_address) {
  if (crypto_config_.NumberOfConfigs() == 0) {
    FLOG(ERROR) << "QUIC server config is not set";
    return false;
  }

  for (size_t i = 0; i < worker_size; ++i) {
    std::unique_ptr<ThreadWorker> worker(
        new ThreadWorker(quic_address,
                         quic_config_,
                         &crypto_config_,
                         server_config_,
                         supported_versions_));
    worker->Start();
    worker_list_.push_back(std::move(worker));
  }

  return true;
}

bool QuicThreadServer::Shutdown() {
  for (size_t i = 0; i < worker_list_.size(); ++i) {
    worker_list_[i]->Stop();
  }

  worker_list_.clear();
  return true;
}

void QuicThreadServer::SetStrikeRegisterNoStartupPeriod() {
  crypto_config_.set_strike_register_no_startup_period();
}

void QuicThreadServer::SetEphemeralKeySource(EphemeralKeySource* key_source) {
  crypto_config_.SetEphemeralKeySource(key_source);
}

bool QuicThreadServer::Initialize(
    const std::vector<QuicServerConfigProtobuf*>& protobufs) {
  // If an initial flow control window has not explicitly been set, then use a
  // sensible value for a server: 1 MB for session, 64 KB for each stream.
  const uint32_t kInitialSessionFlowControlWindow = 1 * 1024 * 1024;  // 1 MB
  const uint32_t kInitialStreamFlowControlWindow = 64 * 1024;         // 64 KB
  if (quic_config_.GetInitialStreamFlowControlWindowToSend() ==
      kMinimumFlowControlSendWindow) {
    quic_config_.SetInitialStreamFlowControlWindowToSend(
        kInitialStreamFlowControlWindow);
  }

  if (quic_config_.GetInitialSessionFlowControlWindowToSend() ==
      kMinimumFlowControlSendWindow) {
    quic_config_.SetInitialSessionFlowControlWindowToSend(
        kInitialSessionFlowControlWindow);
  }
  crypto_config_.set_strike_register_no_startup_period();

  if (protobufs.size()) {
    if (!crypto_config_.SetConfigs(protobufs, QuicWallTime::Zero())) {
      FLOG(ERROR) << "Failed to set QUIC server config";
      return false;
    }
  } else {
    std::unique_ptr<CryptoHandshakeMessage> scfg(
        crypto_config_.AddDefaultConfig(
            QuicRandom::GetInstance(),
            &clock_,
            QuicCryptoServerConfig::ConfigOptions()));
  }

  return true;
}

} // namespace net
