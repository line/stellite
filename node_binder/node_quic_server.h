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

#ifndef NODE_BINDER_NODE_QUIC_SERVER_H_
#define NODE_BINDER_NODE_QUIC_SERVER_H_

#include <memory>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "net/base/ip_endpoint.h"
#include "net/quic/core/crypto/quic_crypto_server_config.h"
#include "net/quic/core/quic_clock.h"
#include "net/quic/core/quic_config.h"
#include "node/uv.h"
#include "stellite/include/stellite_export.h"

namespace net {

class IOBufferWithSize;

}  // namespace net

namespace stellite {

class NodeQuicDispatcher;
class NodeQuicNotifyInterface;
class UDPServer;

class NodeQuicServer {
 public:
  NodeQuicServer(const std::string& cert_path, const std::string& key_path,
                 NodeQuicNotifyInterface* notify_interface);
  ~NodeQuicServer();

  bool Initialize();
  void Shutdown();

  // listen a specified address
  int Listen(const net::IPEndPoint& address);

private:
  void StartReading();
  void OnReadComplete(int result);

  const std::string cert_path_;
  const std::string key_path_;
  NodeQuicNotifyInterface* notify_interface_;

  // quic dispatcher
  net::QuicClock quic_clock_;
  net::QuicConfig quic_config_;
  std::unique_ptr<net::QuicCryptoServerConfig> crypto_server_config_;
  std::unique_ptr<net::QuicVersionManager> version_manager_;
  std::unique_ptr<NodeQuicDispatcher> dispatcher_;

  // server address that listens on
  net::IPEndPoint server_address_;

  bool read_pending_;

  int synchronous_read_count_;

  scoped_refptr<net::IOBufferWithSize> read_buffer_;

  // source address of current read
  net::IPEndPoint client_address_;

  // udp server
  std::unique_ptr<UDPServer> udp_server_;

  DISALLOW_COPY_AND_ASSIGN(NodeQuicServer);
};

}  // namespace net

#endif  // NODE_BINDER_NODE_QUIC_SERVER_H_
