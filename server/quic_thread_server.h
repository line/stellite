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

#ifndef STELLITE_THREAD_QUIC_SERVER_H_
#define STELLITE_THREAD_QUIC_SERVER_H_

#include <map>
#include <memory>
#include <vector>

#include "stellite/server/server_config.h"
#include "stellite/server/thread_worker.h"
#include "net/base/ip_endpoint.h"
#include "net/quic/quic_clock.h"
#include "net/quic/quic_config.h"

namespace base {
class Thread;
} // namespace thread

namespace net {
class QuicServerConfigProtobuf;
class SharedSessionManager;

class QuicThreadServer {
 public:
  QuicThreadServer(const QuicConfig& quic_config,
                   const ServerConfig& server_config,
                   const QuicVersionVector& supported_version);
  virtual ~QuicThreadServer();

  bool Initialize();

  bool Start(size_t worker_size, const IPEndPoint& quic_address,
             std::vector<QuicServerConfigProtobuf*> serialized_config);

  bool Shutdown();

 private:
  typedef std::map<QuicConnectionId, base::PlatformThreadId> ConnectionMap;
  typedef std::vector<std::unique_ptr<ThreadWorker>> WorkerList;

  // QUIC clock
  QuicClock clock_;

  // QUIC config
  QuicConfig quic_config_;

  // Server config
  const ServerConfig& server_config_;

  // List of supported QUIC versions
  QuicVersionVector supported_versions_;

  // Worker container
  WorkerList worker_list_;

  // Connection container
  ConnectionMap connection_map_;

  DISALLOW_COPY_AND_ASSIGN(QuicThreadServer);
};

} // namespace net

#endif // STELLITE_THREAD_QUIC_SERVER_H_
