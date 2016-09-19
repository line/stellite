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

#include "stellite/test/quic_mock_server.h"

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/threading/thread.h"
#include "stellite/server/quic_thread_server.h"
#include "stellite/server/server_config.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/quic/crypto/proof_source_chromium.h"

QuicMockServer::QuicMockServer()
  : proxy_timeout_(0) {
}

QuicMockServer::~QuicMockServer() {}

bool QuicMockServer::Start(uint16_t port) {
  server_config_.reset(new net::ServerConfig());

  if (proxy_pass_.size()) {
    server_config_->proxy_pass(proxy_pass_);
  }

  if (proxy_timeout_) {
    server_config_->proxy_timeout(proxy_timeout_);
  }

  CHECK(!certfile_.empty() && !keyfile_.empty())
      << "certfile or keyfile is required";

  net::ProofSourceChromium* proof_source = new net::ProofSourceChromium();
  CHECK(proof_source->Initialize(certfile_, keyfile_, base::FilePath()))
      << "Failed to initialize the certificate";

  net::QuicConfig quic_config;
  quic_server_.reset(new net::QuicThreadServer(quic_config,
                                               *server_config_.get(),
                                               net::QuicSupportedVersions(),
                                               proof_source));

  std::vector<net::QuicServerConfigProtobuf*> quic_server_configs;
  CHECK(quic_server_->Initialize(quic_server_configs));

  net::IPAddress ip;
  CHECK(ip.AssignFromIPLiteral("::"));
  quic_server_->Start(/* worker_size */ 1, net::IPEndPoint(ip, port));

  return true;
}

bool QuicMockServer::Shutdown() {
  quic_server_->Shutdown();
  return true;
}

bool QuicMockServer::SetCertificate(const std::string& keyfile,
                                    const std::string& certfile) {
  base::FilePath root_path;
  if (!PathService::Get(base::DIR_SOURCE_ROOT, &root_path)) {
    LOG(ERROR) << "Failed to get base::DIR_SOURCE_ROOT";
    return false;
  }

  base::FilePath res_path = root_path.Append(FILE_PATH_LITERAL("stellite")).
                                      Append(FILE_PATH_LITERAL("test_tools")).
                                      Append(FILE_PATH_LITERAL("res"));

  return SetCertificate(res_path.AppendASCII(keyfile),
                        res_path.AppendASCII(certfile));
}

bool QuicMockServer::SetCertificate(const base::FilePath& keyfile,
                                    const base::FilePath& certfile) {
  keyfile_ = keyfile;
  certfile_ = certfile;
  return true;
}

bool QuicMockServer::SetProxyPass(const std::string& proxy_url) {
  proxy_pass_ = proxy_url;
  return true;
}

bool QuicMockServer::SetProxyTimeout(uint32_t proxy_timeout) {
  proxy_timeout_ = proxy_timeout;
  return true;
}

