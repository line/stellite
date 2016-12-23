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
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/quic/chromium/crypto/proof_source_chromium.h"
#include "net/quic/chromium/quic_chromium_alarm_factory.h"
#include "net/quic/chromium/quic_chromium_connection_helper.h"
#include "net/quic/core/crypto/quic_crypto_server_config.h"
#include "net/quic/core/quic_protocol.h"
#include "stellite/fetcher/http_request_context_getter.h"
#include "stellite/server/quic_proxy_dispatcher.h"
#include "stellite/server/quic_proxy_server.h"
#include "stellite/server/server_config.h"

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

  stellite::HttpRequestContextGetter::Params fetcher_params;
  fetcher_params.enable_http2 = true;
  fetcher_params.enable_quic = true;
  fetcher_params.ignore_certificate_errors = false;
  fetcher_params.using_disk_cache = false;

  // setting QuicConfig
  quic_config_.reset(new net::QuicConfig());

  // setting CryptoConfig
  CHECK(!certfile_.empty() && !keyfile_.empty())
      << "certfile or keyfile is required";

  std::unique_ptr<net::ProofSourceChromium> proof_source(
      new net::ProofSourceChromium());
  CHECK(proof_source->Initialize(certfile_, keyfile_, base::FilePath()))
      << "Failed to initialize the certificate";

  crypto_config_.reset(
      new net::QuicCryptoServerConfig("secret",
                                      net::QuicRandom::GetInstance(),
                                      std::move(proof_source)));

  server_config_.reset(new net::ServerConfig());

  version_manager_.reset(new net::QuicVersionManager(
          net::AllSupportedVersions()));

  connection_helper_.reset(
      new net::QuicChromiumConnectionHelper(&quic_clock_,
                                            net::QuicRandom::GetInstance()));

  alarm_factory_.reset(new net::QuicChromiumAlarmFactory(
          base::ThreadTaskRunnerHandle::Get().get(),
          &quic_clock_));

  quic_dispatcher_.reset(new net::QuicProxyDispatcher(
          fetcher_params,
          *quic_config_,
          crypto_config_.get(),
          *server_config_,
          version_manager_.get(),
          connection_helper_.get(),
          alarm_factory_.get()));

  return true;
}

bool QuicMockServer::Shutdown() {
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

