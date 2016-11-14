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

#ifndef QUIC_TEST_TOOLS_QUIC_MOCK_SERVER_H_
#define QUIC_TEST_TOOLS_QUIC_MOCK_SERVER_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/synchronization/waitable_event.h"

namespace base {
class Thread;
} // namespace base

namespace net {
class QuicProofSource;
class QuicThreadServer;
class ServerConfig;
} // namespace net

class QuicMockServer {
 public:
  QuicMockServer();
  ~QuicMockServer();

  bool Start(uint16_t port);

  bool Shutdown();

  bool SetCertificate(const base::FilePath& key_path,
                      const base::FilePath& cert_path);

  bool SetCertificate(const std::string& key_path,
                      const std::string& cert_path);

  bool SetProxyPass(const std::string& proxy_url);
  bool SetProxyTimeout(uint32_t proxy_timeout);

 private:
  std::unique_ptr<net::QuicThreadServer> quic_server_;

  std::unique_ptr<net::ServerConfig> server_config_;

  base::FilePath keyfile_;
  base::FilePath certfile_;

  std::string proxy_pass_;
  uint32_t proxy_timeout_;

  DISALLOW_COPY_AND_ASSIGN(QuicMockServer);
};

#endif // QUIC_TEST_TOOLS_QUIC_MOCK_SERVER_H_
