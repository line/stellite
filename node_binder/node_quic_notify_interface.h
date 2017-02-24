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

#ifndef NODE_BINDER_NODE_QUIC_NOTIFY_INTERFACE_H_
#define NODE_BINDER_NODE_QUIC_NOTIFY_INTERFACE_H_

#include "net/quic/core/quic_protocol.h"
#include "net/spdy/spdy_header_block.h"
#include "stellite/include/stellite_export.h"

namespace stellite {

class NodeQuicServerSession;
class NodeQuicServerStream;

class NodeQuicNotifyInterface {
 public:
  virtual ~NodeQuicNotifyInterface() {}

  // called when stream are created
  virtual void OnStreamCreated(NodeQuicServerSession* session,
                               NodeQuicServerStream* stream) = 0;

  // called when stream are closed
  virtual void OnStreamClosed(NodeQuicServerSession* session,
                              NodeQuicServerStream* stream) = 0;

  virtual void OnSessionCreated(NodeQuicServerSession* session) = 0;

  // called when connection are closed
  virtual void OnSessionClosed(NodeQuicServerSession* session,
                               net::QuicErrorCode error,
                               const std::string& error_details,
                               net::ConnectionCloseSource source) = 0;
};

}  // namespace net

#endif  // NODE_BINDER_NODE_QUIC_NOTIFY_INTERFACE_H_
