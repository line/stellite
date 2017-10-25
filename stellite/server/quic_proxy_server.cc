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

#include "stellite/server/quic_proxy_server.h"

#include "base/memory/ptr_util.h"
#include "base/threading/thread.h"
#include "net/quic/chromium/crypto/proof_source_chromium.h"
#include "stellite/crypto/quic_ephemeral_key_source.h"
#include "stellite/server/quic_proxy_worker.h"

namespace net {

class ServerPacketWriter;
const char* kWorkerThread = "worker thread";
const char* kFetcherThread = "fetcher thread";

QuicProxyServer::QuicProxyServer(const QuicConfig& quic_config,
                                 const ServerConfig& server_config,
                                 const QuicVersionVector& supported_versions)
    : quic_config_(quic_config),
      server_config_(server_config),
      supported_versions_(supported_versions) {
}

QuicProxyServer::~QuicProxyServer() {}

bool QuicProxyServer::Start(size_t worker_size,
    const IPEndPoint& quic_address,
    std::vector<QuicServerConfigProtobuf*> serialized_config) {

  for (size_t i = 0; i < worker_size; ++i) {
    std::unique_ptr<ProofSourceChromium> proof_source(
        new ProofSourceChromium());
    if (!proof_source->Initialize(server_config_.certfile(),
                                  server_config_.keyfile(),
                                  base::FilePath())) {
      LOG(ERROR) << "Failed to parse the certificate";
      return false;
    }

    base::Thread::Options io_options(base::MessageLoop::TYPE_IO, 0);
    base::Thread* dispatch_thread = new base::Thread(kWorkerThread);
    dispatch_thread->StartWithOptions(io_options);

    base::Thread* fetch_thread = new base::Thread(kFetcherThread);
    fetch_thread->StartWithOptions(io_options);

    QuicProxyWorker* worker = new QuicProxyWorker(
        fetch_thread->task_runner(),
        dispatch_thread->task_runner(),
        quic_address,
        quic_config_,
        server_config_,
        supported_versions_,
        std::move(proof_source));

    worker->SetStrikeRegisterNoStartupPeriod();

    // Ephemeral key source, owned by worker
    worker->SetEphemeralKeySource(new QuicEphemeralKeySource());

    if (!worker->Initialize(serialized_config)) {
      LOG(ERROR) << "failed to parse quic server config";
      return false;
    }

    dispatch_thread_list_.push_back(base::WrapUnique(dispatch_thread));
    fetch_thread_list_.push_back(base::WrapUnique(fetch_thread));
    worker_list_.push_back(base::WrapUnique(worker));

    worker->Start();
  }

  return true;
}

bool QuicProxyServer::Shutdown() {
  for (size_t i = 0; i < worker_list_.size(); ++i) {
    worker_list_[i]->Stop();
  }

  worker_list_.clear();
  return true;
}

bool QuicProxyServer::Initialize() {
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

  return true;
}

} // namespace net
