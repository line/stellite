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

#ifndef NODE_BINDER_NODE_QUIC_DISPATCHER_H_
#define NODE_BINDER_NODE_QUIC_DISPATCHER_H_

#include <memory>

#include "net/tools/quic/quic_dispatcher.h"
#include "stellite/include/stellite_export.h"

namespace stellite {

class NodeQuicNotifyInterface;

class NodeQuicDispatcher : public net::QuicDispatcher {
 public:
  NodeQuicDispatcher(
      const net::QuicConfig& config,
      const net::QuicCryptoServerConfig* crypto_config,
      net::QuicVersionManager* version_manager,
      std::unique_ptr<net::QuicConnectionHelperInterface> helper,
      std::unique_ptr<net::QuicCryptoServerStream::Helper> session_helper,
      std::unique_ptr<net::QuicAlarmFactory> alarm_factory,
      NodeQuicNotifyInterface* notify_interface);
  ~NodeQuicDispatcher() override;

 protected:
  net::QuicServerSessionBase* CreateQuicSession(
      net::QuicConnectionId connection_id,
      const net::IPEndPoint& client_address) override;

 private:
  NodeQuicNotifyInterface* notify_interface_;

  DISALLOW_COPY_AND_ASSIGN(NodeQuicDispatcher);
};

}  // namespace net

#endif  // NODE_BINDER_NODE_QUIC_DISPATCHER_H_
