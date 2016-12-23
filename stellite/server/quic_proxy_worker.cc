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

#include "stellite/server/quic_proxy_worker.h"

#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "net/base/net_errors.h"
#include "net/quic/chromium/quic_chromium_alarm_factory.h"
#include "net/quic/chromium/quic_chromium_connection_helper.h"
#include "net/quic/core/crypto/quic_random.h"
#include "net/quic/core/quic_packet_writer.h"
#include "net/quic/core/quic_protocol.h"
#include "net/tools/quic/quic_dispatcher.h"
#include "stellite/crypto/quic_ephemeral_key_source.h"
#include "stellite/fetcher/http_request_context_getter.h"
#include "stellite/server/quic_proxy_dispatcher.h"
#include "stellite/server/server_config.h"
#include "stellite/server/server_packet_writer.h"
#include "stellite/socket/quic_udp_server_socket.h"

namespace net {

const int kReadBufferSize = 2 * kMaxPacketSize;
const size_t kNumSessionsToCreatePerSocketEvent = 16;
const char* kSourceAddressTokenSecret = "secret";

QuicProxyWorker::QuicProxyWorker(
    scoped_refptr<base::SingleThreadTaskRunner> dispatch_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> http_fetch_task_runner,
    const IPEndPoint& bind_address,
    const QuicConfig& quic_config,
    const ServerConfig& server_config,
    const QuicVersionVector& supported_versions,
    std::unique_ptr<ProofSource> proof_source)
    : dispatch_continuity_(server_config.dispatch_continuity()),
      dispatch_task_runner_(dispatch_task_runner),
      http_fetch_task_runner_(http_fetch_task_runner),
      bind_address_(bind_address),
      quic_config_(quic_config),
      crypto_config_(kSourceAddressTokenSecret,
                     QuicRandom::GetInstance(),
                     std::move(proof_source)),
      server_config_(server_config),
      version_manager_(supported_versions),
      read_pending_(false),
      synchronous_read_count_(0),
      read_buffer_(new IOBufferWithSize(kReadBufferSize)),
      weak_factory_(this) {
      CHECK(dispatch_continuity_ >= 1 && dispatch_continuity_ <= 32) <<
          "keep dispatch_continuity range [1, 32]";
    }

QuicProxyWorker::~QuicProxyWorker() {}

bool QuicProxyWorker::Initialize() {
  std::unique_ptr<CryptoHandshakeMessage> scfg(
      crypto_config_.AddDefaultConfig(
          QuicRandom::GetInstance(),
          &clock_,
          QuicCryptoServerConfig::ConfigOptions()));
  return true;
}

bool QuicProxyWorker::Initialize(
    const std::vector<QuicServerConfigProtobuf*>& protobufs) {
  if (protobufs.size()) {
    if (!crypto_config_.SetConfigs(protobufs, QuicWallTime::Zero())) {
      LOG(ERROR) << "Failed to set QUIC server config";
      return false;
    }
  } else {
    return Initialize();
  }

  return true;
}

void QuicProxyWorker::SetStrikeRegisterNoStartupPeriod() {
  crypto_config_.set_strike_register_no_startup_period();
}

void QuicProxyWorker::SetEphemeralKeySource(EphemeralKeySource* key_source) {
  crypto_config_.SetEphemeralKeySource(key_source);
}

void QuicProxyWorker::Start() {
  DCHECK(crypto_config_.NumberOfConfigs() > 0);
  dispatch_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&QuicProxyWorker::StartOnBackground,
                 weak_factory_.GetWeakPtr()));
}

void QuicProxyWorker::StartOnBackground() {
  DCHECK(dispatch_task_runner_->BelongsToCurrentThread());

  helper_ = new QuicChromiumConnectionHelper(&clock_,
                                             QuicRandom::GetInstance());
  alarm_factory_ = new QuicChromiumAlarmFactory(
      base::ThreadTaskRunnerHandle::Get().get(),
      &clock_);

  std::unique_ptr<QuicUDPServerSocket> socket(new QuicUDPServerSocket());

  socket->AllowAddressReuse();
  socket->AllowPortReuse();

  int res = socket->Listen(bind_address_);
  if (res < 0) {
    LOG(ERROR) << "Listen() failed: " << ErrorToString(res);
    exit(1);
  }

  // These send and receive buffer sizes are sized for a single connection,
  // because the default usage of QuicProxyServer is as a test server with
  // one or two clients.  Adjust higher for using with many clients.
  res = socket->SetReceiveBufferSize(
      static_cast<int>(server_config().recv_buffer_size()));
  if (res < 0) {
    LOG(ERROR) << "SetReceiveBufferSize() failed: " << ErrorToString(res);
    exit(1);
  }

  res = socket->SetSendBufferSize(server_config().send_buffer_size());
  if (res < 0) {
    LOG(ERROR) << "SetSendBufferSize() failed: " << ErrorToString(res);
    exit(1);
  }

  res = socket->GetLocalAddress(&server_address_);
  if (res < 0) {
    LOG(ERROR) << "GetLocalAddress() failed: " << ErrorToString(res);
    exit(1);
  }

  LOG(INFO) << "Listening on " << server_address_.ToString();

  socket_.swap(socket);

  stellite::HttpRequestContextGetter::Params fetcher_params;
  fetcher_params.enable_http2 = true;
  fetcher_params.enable_quic = false;
  fetcher_params.ignore_certificate_errors = false;
  fetcher_params.using_disk_cache = false;

  dispatcher_.reset(
      new QuicProxyDispatcher(fetcher_params, http_fetch_task_runner(),
                              quic_config(), crypto_config(), server_config(),
                              &version_manager_, helper_, alarm_factory_));

  ServerPacketWriter* writer = new ServerPacketWriter(socket_.get(),
                                                      dispatcher_.get());
  dispatcher_->InitializeWithWriter(writer);

  StartReading();
}

void QuicProxyWorker::Stop() {
  dispatch_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&QuicProxyWorker::StopReading,
                 weak_factory_.GetWeakPtr()));
}

void QuicProxyWorker::StartReading() {
  if (synchronous_read_count_ == 0) {
    dispatcher_->ProcessBufferedChlos(kNumSessionsToCreatePerSocketEvent);
  }

  if (read_pending_) {
    return;
  }
  read_pending_ = true;

  int result = socket_->RecvFrom(
      read_buffer_.get(),
      read_buffer_->size(),
      &client_address_,
      base::Bind(&QuicProxyWorker::OnReadComplete, base::Unretained(this)));

  if (result == ERR_IO_PENDING) {
    synchronous_read_count_ = 0;
    if (dispatcher_->HasChlosBuffered()) {
      dispatch_task_runner_->PostTask(
          FROM_HERE,
          base::Bind(&QuicProxyWorker::StartReading,
                     weak_factory_.GetWeakPtr()));
    }
    return;
  }

  if (++synchronous_read_count_ > dispatch_continuity_) {
    synchronous_read_count_ = 0;
    dispatch_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&QuicProxyWorker::OnReadComplete,
                   weak_factory_.GetWeakPtr(),
                   result));
  } else {
    OnReadComplete(result);
  }
}

void QuicProxyWorker::StopReading() {
  dispatcher_->Shutdown();

  socket_->Close();
  socket_.reset();
}

void QuicProxyWorker::OnReadComplete(int result) {
  read_pending_ = false;

  if (result == 0) {
    result = ERR_CONNECTION_CLOSED;
  }

  if (result < 0) {
    LOG(ERROR) << "QuicProxyWorker read failed: " << ErrorToString(result);
    Stop();
    return;
  }

  QuicReceivedPacket packet(read_buffer_->data(), result,
                            helper_->GetClock()->Now(), false);
  dispatcher_->ProcessPacket(server_address_, client_address_, packet);

  StartReading();
}

} // namespace net
