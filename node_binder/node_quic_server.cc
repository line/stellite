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

#include "node_binder/node_quic_server.h"

#include <vector>

#include "base/message_loop/message_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/quic/chromium/crypto/proof_source_chromium.h"
#include "net/quic/chromium/quic_chromium_connection_helper.h"
#include "net/quic/core/crypto/crypto_server_config_protobuf.h"
#include "net/tools/quic/quic_simple_server_session_helper.h"
#include "node_binder/node_message_pump_manager.h"
#include "node_binder/node_quic_alarm_factory.h"
#include "node_binder/node_quic_dispatcher.h"
#include "node_binder/node_quic_server_packet_writer.h"
#include "node_binder/quic_server_config_util.h"
#include "node_binder/socket/udp_server.h"
#include "stellite/crypto/quic_ephemeral_key_source.h"

namespace stellite {

namespace {

const char kSourceAddressTokenSecret[] = "secret";
const net::QuicByteCount kSocketReceiveBufferSize = 1024 * 1024 * 4; // 4M
const net::QuicByteCount kSocketSendBufferSize  = 20 * net::kMaxPacketSize;
const int kReadBufferSize = 2 * net::kMaxPacketSize;

std::unique_ptr<net::ProofSource> CreateProofSource(
    const std::string& cert_path,
    const std::string& key_path) {
  std::unique_ptr<net::ProofSourceChromium> proof_source(
      new net::ProofSourceChromium());
  CHECK(proof_source->Initialize(base::FilePath(cert_path),
                                 base::FilePath(key_path),
                                 base::FilePath()));
  return std::move(proof_source);
}

}  // namespace anonymouse

NodeQuicServer::NodeQuicServer(const std::string& cert_path,
                               const std::string& key_path,
                               const std::string& hex_encoded_config,
                               NodeQuicNotifyInterface* notify_interface)
    : cert_path_(cert_path),
      key_path_(key_path),
      hex_encoded_config_(hex_encoded_config),
      notify_interface_(notify_interface),
      read_pending_(false),
      synchronous_read_count_(0),
      read_buffer_(new net::IOBufferWithSize(kReadBufferSize)) {
}

NodeQuicServer::~NodeQuicServer() {}

bool NodeQuicServer::Initialize() {
  // If an initial flow control window has not explicitly been set, then use a
  // sensible value for a server: 1 MB for session, 64 KB for each stream.
  const uint32_t kInitialSessionFlowControlWindow = 1 * 1024 * 1024;  // 1 MB
  const uint32_t kInitialStreamFlowControlWindow = 64 * 1024;         // 64 KB
  if (quic_config_.GetInitialStreamFlowControlWindowToSend() ==
      net::kMinimumFlowControlSendWindow) {
    quic_config_.SetInitialStreamFlowControlWindowToSend(
        kInitialStreamFlowControlWindow);
  }
  if (quic_config_.GetInitialSessionFlowControlWindowToSend() ==
      net::kMinimumFlowControlSendWindow) {
    quic_config_.SetInitialSessionFlowControlWindowToSend(
        kInitialSessionFlowControlWindow);
  }

  crypto_server_config_.reset(new net::QuicCryptoServerConfig(
          kSourceAddressTokenSecret,
          net::QuicRandom::GetInstance(),
          CreateProofSource(cert_path_, key_path_)));

  crypto_server_config_->set_strike_register_no_startup_period();

  crypto_server_config_->SetEphemeralKeySource(
      new net::QuicEphemeralKeySource());

  // TODO(@snibug) if quic server config are cached take it and initialize with
  // crypto_server_config_->SetConfigs() interface
  std::unique_ptr<net::CryptoHandshakeMessage> scfg;
  if (hex_encoded_config_.size()) {
    std::unique_ptr<net::QuicServerConfigProtobuf> config_protobuf(
        QuicServerConfigUtil::DecodeConfig(hex_encoded_config_));
    if (!config_protobuf.get()) {
      LOG(ERROR) << "fail to decode quic server config";
      return false;
    }

    scfg.reset(crypto_server_config_->AddConfig(
            config_protobuf.get(),
            quic_clock_.WallNow()));
  } else {
    scfg.reset(
        crypto_server_config_->AddDefaultConfig(
            net::QuicRandom::GetInstance(),
            &quic_clock_,
            net::QuicCryptoServerConfig::ConfigOptions()));
  }

  if (!scfg.get()) {
    LOG(ERROR) << "failed to parse quic server config message";
    return false;
  }

  version_manager_.reset(new net::QuicVersionManager(
          net::AllSupportedVersions()));

  return true;
}

int NodeQuicServer::Listen(const net::IPEndPoint& address) {
  std::unique_ptr<net::QuicChromiumConnectionHelper> connection_helper(
      new net::QuicChromiumConnectionHelper(&quic_clock_,
                                       net::QuicRandom::GetInstance()));

  std::unique_ptr<net::QuicCryptoServerStream::Helper> session_helper(
      new net::QuicSimpleServerSessionHelper(net::QuicRandom::GetInstance()));

  std::unique_ptr<net::QuicAlarmFactory> alarm_factory(
      new NodeQuicAlarmFactory(&quic_clock_));

  std::unique_ptr<NodeQuicDispatcher> dispatcher(new NodeQuicDispatcher(
          quic_config_,
          crypto_server_config_.get(),
          version_manager_.get(),
          std::move(connection_helper),
          std::move(session_helper),
          std::move(alarm_factory),
          notify_interface_));

  std::unique_ptr<UDPServer> udp_server(new UDPServer());
  net::AddressFamily address_family = GetAddressFamily(address.address());
  int rv = udp_server->Open(address_family);
  if (rv != net::OK) {
    LOG(ERROR) << "udp server open failed: " << rv;
    return rv;
  }

  rv = udp_server->AllowAddressReuse();
  if (rv != net::OK) {
    LOG(ERROR) << "udp server reuse address are failed: " << rv;
    return rv;
  }

  rv = udp_server->AllowPortReuse();
  if (rv != net::OK) {
    LOG(ERROR) << "udp server reuse port are failed: " << rv;
    return rv;
  }

  rv = udp_server->SetReceiveBufferSize(kSocketReceiveBufferSize);
  if (rv != net::OK) {
    LOG(ERROR) << "udp server set receive buffer size are failed: " << rv;
    return rv;
  }

  rv = udp_server->SetSendBufferSize(kSocketSendBufferSize);
  if (rv != net::OK) {
    LOG(ERROR) << "udp server set send buffer size are failed: " << rv;
    return rv;
  }

  rv = udp_server->Listen(address);
  if (rv != net::OK) {
    LOG(ERROR) << "udp server listen failed: " << rv;
    return rv;
  }

  // accquire node's message_loop reference count that keep a node's message
  // loop lifecycle
  DCHECK(NodeMessagePumpManager::current());
  NodeMessagePumpManager::current()->AddRef();

  NodeQuicServerPacketWriter* writer =
      new NodeQuicServerPacketWriter(udp_server.get());
  dispatcher->InitializeWithWriter(writer);

  dispatcher_ = std::move(dispatcher);
  udp_server_ = std::move(udp_server);

  StartReading();

  return net::OK;
}

void NodeQuicServer::Shutdown() {
  if (!dispatcher_) {
    return;
  }
  dispatcher_->Shutdown();
  dispatcher_.reset();

  DCHECK(NodeMessagePumpManager::current());
  NodeMessagePumpManager::current()->RemoveRef();
}

void NodeQuicServer::StartReading() {
  if (synchronous_read_count_ == 0) {
    dispatcher_->ProcessBufferedChlos(4);
  }

  if (read_pending_) {
    return;
  }
  read_pending_ = true;

  int result = udp_server_->RecvFrom(
      read_buffer_.get(), read_buffer_->size(), &client_address_,
      base::Bind(&NodeQuicServer::OnReadComplete, base::Unretained(this)));

  if (result == net::ERR_IO_PENDING) {
    synchronous_read_count_ = 0;
    if (dispatcher_->HasChlosBuffered()) {
      base::ThreadTaskRunnerHandle::Get()->PostTask(
          FROM_HERE,
          base::Bind(&NodeQuicServer::StartReading,
                     base::Unretained(this)));
    }
    return;
  }

  if (++synchronous_read_count_ > 4) {
    synchronous_read_count_ = 0;
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::Bind(&NodeQuicServer::OnReadComplete,
                   base::Unretained(this),
                   result));
  } else {
    OnReadComplete(result);
  }
}

void NodeQuicServer::OnReadComplete(int result) {
  read_pending_ = false;
  if (result == 0) {
    result = net::ERR_CONNECTION_CLOSED;
    LOG(ERROR) << "connection are closed";
    Shutdown();
    return;
  }

  if (result < 0) {
    LOG(ERROR) << "NodeQuicServer read failed: " << net::ErrorToString(result);
    Shutdown();
    return;
  }

  net::QuicReceivedPacket packet(read_buffer_->data(), result, quic_clock_.Now(),
                            false);
  dispatcher_->ProcessPacket(server_address_, client_address_, packet);

  StartReading();
}

}  // namespace stellite
