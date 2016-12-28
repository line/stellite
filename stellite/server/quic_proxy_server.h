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

#ifndef STELLITE_SERVER_QUIC_PROXY_SERVER_H_
#define STELLITE_SERVER_QUIC_PROXY_SERVER_H_

#include <map>
#include <memory>
#include <vector>

#include "base/threading/platform_thread.h"
#include "net/base/ip_endpoint.h"
#include "net/quic/core/quic_clock.h"
#include "net/quic/core/quic_config.h"
#include "stellite/include/stellite_export.h"
#include "stellite/server/server_config.h"

namespace base {
class Thread;
} // namespace thread

namespace net {
class QuicServerConfigProtobuf;
class SharedSessionManager;
class QuicProxyWorker;

class STELLITE_EXPORT QuicProxyServer {
 public:
  QuicProxyServer(const QuicConfig& quic_config,
                  const ServerConfig& server_config,
                  const QuicVersionVector& supported_version);
  virtual ~QuicProxyServer();

  bool Initialize();

  bool Start(size_t worker_size, const IPEndPoint& quic_address,
             std::vector<QuicServerConfigProtobuf*> serialized_config);

  bool Shutdown();

 private:
  typedef std::map<QuicConnectionId, base::PlatformThreadId> ConnectionMap;
  typedef std::vector<std::unique_ptr<QuicProxyWorker>> WorkerList;
  typedef std::vector<std::unique_ptr<base::Thread>> ThreadVector;

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

  // threads for worker
  ThreadVector dispatch_thread_list_;

  // threads for fetcher
  ThreadVector fetch_thread_list_;

  DISALLOW_COPY_AND_ASSIGN(QuicProxyServer);
};

} // namespace net

#endif // STELLITE_SERVER_QUIC_PROXY_SERVER_H_
