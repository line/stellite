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

#include "stellite/server/thread_worker.h"

#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "net/base/net_errors.h"
#include "net/quic/crypto/quic_random.h"
#include "net/quic/quic_chromium_alarm_factory.h"
#include "net/quic/quic_chromium_connection_helper.h"
#include "net/quic/quic_packet_writer.h"
#include "net/quic/quic_protocol.h"
#include "net/tools/quic/quic_dispatcher.h"
#include "stellite/crypto/quic_ephemeral_key_source.h"
#include "stellite/fetcher/http_request_context_getter.h"
#include "stellite/logging/logging.h"
#include "stellite/server/server_config.h"
#include "stellite/server/server_packet_writer.h"
#include "stellite/server/thread_dispatcher.h"
#include "stellite/socket/quic_udp_server_socket.h"

namespace net {

const int kReadBufferSize = 2 * kMaxPacketSize;
const char* kDispatchThread = "dispatch thread";
const char* kFetchThread = "fetch thread";
const char* kSourceAddressTokenSecret = "secret";

ThreadWorker::ThreadWorker(
    const IPEndPoint& bind_address,
    const QuicConfig& quic_config,
    const ServerConfig& server_config,
    const QuicVersionVector& supported_versions,
    ProofSource* proof_source)
    : dispatch_continuity_(server_config.dispatch_continuity()),
      bind_address_(bind_address),
      quic_config_(quic_config),
      crypto_config_(kSourceAddressTokenSecret,
                     QuicRandom::GetInstance(),
                     proof_source),
      server_config_(server_config),
      supported_versions_(supported_versions),
      read_pending_(false),
      synchronous_read_count_(0),
      read_buffer_(new IOBufferWithSize(kReadBufferSize)),
      weak_factory_(this) {
  CHECK(dispatch_continuity_ >= 1 && dispatch_continuity_ <= 32) <<
      "keep dispatch_continuity range [1, 32]";
}

ThreadWorker::~ThreadWorker() {}

bool ThreadWorker::Initialize(
    const std::vector<QuicServerConfigProtobuf*>& protobufs) {
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

void ThreadWorker::SetStrikeRegisterNoStartupPeriod() {
  crypto_config_.set_strike_register_no_startup_period();
}

void ThreadWorker::SetEphemeralKeySource(EphemeralKeySource* key_source) {
  crypto_config_.SetEphemeralKeySource(key_source);
}

void ThreadWorker::Start() {
  CHECK(!dispatch_thread_.get());
  CHECK(!fetch_thread_.get());
  CHECK(crypto_config_.NumberOfConfigs() > 0);

  base::Thread::Options worker_thread_option(base::MessageLoop::TYPE_IO, 0);

  dispatch_thread_.reset(new base::Thread(kDispatchThread));
  dispatch_thread_->StartWithOptions(worker_thread_option);

  fetch_thread_.reset(new base::Thread(kFetchThread));
  fetch_thread_->StartWithOptions(worker_thread_option);

  dispatch_task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&ThreadWorker::StartOnBackground,
                   weak_factory_.GetWeakPtr()));
}

void ThreadWorker::StartOnBackground() {
  CHECK(dispatch_thread_->task_runner() ==
        base::ThreadTaskRunnerHandle::Get());

  helper_ = new QuicChromiumConnectionHelper(&clock_,
                                             QuicRandom::GetInstance());
  alarm_factory_ = new QuicChromiumAlarmFactory(
      base::ThreadTaskRunnerHandle::Get().get(),
      &clock_);

  std::unique_ptr<QuicUDPServerSocket> socket(
      new QuicUDPServerSocket(&net_log_, NetLog::Source()));

  socket->AllowAddressReuse();
  socket->AllowPortReuse();

  int res = socket->Listen(bind_address_);
  if (res < 0) {
    FLOG(ERROR) << "Listen() failed: " << ErrorToString(res);
    exit(1);
  }

  // These send and receive buffer sizes are sized for a single connection,
  // because the default usage of QuicProxyServer is as a test server with
  // one or two clients.  Adjust higher for using with many clients.
  res = socket->SetReceiveBufferSize(
      static_cast<int>(server_config().recv_buffer_size()));
  if (res < 0) {
    FLOG(ERROR) << "SetReceiveBufferSize() failed: " << ErrorToString(res);
    exit(1);
  }

  res = socket->SetSendBufferSize(server_config().send_buffer_size());
  if (res < 0) {
    FLOG(ERROR) << "SetSendBufferSize() failed: " << ErrorToString(res);
    exit(1);
  }

  res = socket->GetLocalAddress(&server_address_);
  if (res < 0) {
    FLOG(ERROR) << "GetLocalAddress() failed: " << ErrorToString(res);
    exit(1);
  }

  FLOG(INFO) << "Listening on " << server_address_.ToString();

  socket_.swap(socket);

  dispatcher_.reset(new ThreadDispatcher(
      fetch_task_runner(),
      quic_config(),
      crypto_config(),
      server_config(),
      supported_versions(),
      helper_,
      alarm_factory_));

  ServerPacketWriter* writer = new ServerPacketWriter(socket_.get(),
                                                      dispatcher_.get());
  dispatcher_->InitializeWithWriter(writer);

  StartReading();
}

void ThreadWorker::Stop() {
  dispatch_task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&ThreadWorker::StopReading,
                 weak_factory_.GetWeakPtr()));
}

void ThreadWorker::StartReading() {
  if (read_pending_) {
    return;
  }
  read_pending_ = true;

  int result = socket_->RecvFrom(
      read_buffer_.get(),
      read_buffer_->size(),
      &client_address_,
      base::Bind(&ThreadWorker::OnReadComplete, base::Unretained(this)));

  if (result == ERR_IO_PENDING) {
    synchronous_read_count_ = 0;
    return;
  }

  if (++synchronous_read_count_ > dispatch_continuity_) {
    synchronous_read_count_ = 0;
    dispatch_task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&ThreadWorker::OnReadComplete,
                   weak_factory_.GetWeakPtr(),
                   result));
  } else {
    OnReadComplete(result);
  }
}

void ThreadWorker::StopReading() {
  dispatcher_->Shutdown();

  socket_->Close();
  socket_.reset();
}

void ThreadWorker::OnReadComplete(int result) {
  read_pending_ = false;

  if (result == 0) {
    result = ERR_CONNECTION_CLOSED;
  }

  if (result < 0) {
    FLOG(ERROR) << "ThreadWorker read failed: " << ErrorToString(result);
    Stop();
    return;
  }

  QuicReceivedPacket packet(read_buffer_->data(), result,
                            helper_->GetClock()->Now(), false);
  dispatcher_->ProcessPacket(server_address_, client_address_, packet);

  StartReading();
}

scoped_refptr<base::SingleThreadTaskRunner>
ThreadWorker::dispatch_task_runner() {
  return dispatch_thread_->task_runner();
}

scoped_refptr<base::SingleThreadTaskRunner> ThreadWorker::fetch_task_runner() {
  return fetch_thread_->task_runner();
}

} // namespace net
