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

#include <memory>

#include "base/base_paths.h"
#include "base/bind_helpers.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/cert/mock_cert_verifier.h"
#include "net/dns/mapped_host_resolver.h"
#include "net/dns/mock_host_resolver.h"
#include "net/quic/chromium/crypto/proof_source_chromium.h"
#include "net/quic/core/quic_clock.h"
#include "net/quic/core/quic_config.h"
#include "net/quic/core/quic_protocol.h"
#include "net/test/cert_test_util.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/test/test_data_directory.h"
#include "stellite/fetcher/http_fetcher.h"
#include "stellite/fetcher/http_fetcher_task.h"
#include "stellite/fetcher/http_request_context_getter.h"
#include "stellite/server/quic_proxy_worker.h"
#include "stellite/server/server_config.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

namespace {

const char* kCertificatePath = "src/net/data/ssl/certificates";

class QuicProxyServerTest : public testing::Test,
                            public stellite::HttpFetcherTask::Visitor {
 public:
  QuicProxyServerTest();
  ~QuicProxyServerTest() override;

  void SetUp() override;
  void TearDown() override;

  bool Initialize(int port);

  void OnTaskComplete(int request_id, const URLFetcher* source,
                      const HttpResponseInfo* response_info) override;

  void OnTaskStream(int request_id, const URLFetcher* source,
                    const HttpResponseInfo* response_info,
                    const char* data, size_t len, bool fin) override;

  void OnTaskError(int request_id, const URLFetcher* source,
                   int error_code) override;

  int last_request_id() {
    return request_id_;
  }

  int last_error_code() {
    return error_code_;
  }

  // quic server
  QuicClock quic_clock_;
  QuicConfig quic_config_;
  ServerConfig server_config_;
  QuicVersionVector version_vector_;
  std::unique_ptr<QuicProxyWorker> proxy_worker_;

  // http fetcher
  scoped_refptr<stellite::HttpRequestContextGetter> request_context_;
  std::unique_ptr<stellite::HttpFetcher> http_fetcher_;

  // http test server
  std::unique_ptr<net::EmbeddedTestServer> test_server_;

  // for synchronize
  base::RunLoop run_loop_;

  int request_id_;
  int error_code_;

  base::WeakPtrFactory<QuicProxyServerTest> weak_factory_;
};

QuicProxyServerTest::QuicProxyServerTest()
    : weak_factory_(this) {
}

QuicProxyServerTest::~QuicProxyServerTest() {}

void QuicProxyServerTest::SetUp() {
}

void QuicProxyServerTest::TearDown() {
  http_fetcher_->CancelAll();

  if (proxy_worker_.get()) {
    proxy_worker_->Stop();
  }
}

void QuicProxyServerTest::OnTaskComplete(
    int request_id, const URLFetcher* source,
    const HttpResponseInfo* response_info) {
  request_id_ = request_id;
  run_loop_.Quit();
}

void QuicProxyServerTest::OnTaskStream(
    int request_id, const URLFetcher* source,
    const HttpResponseInfo* response_info, const char* data, size_t len,
    bool fin) {
  request_id_ = request_id;
  run_loop_.Quit();
}

void QuicProxyServerTest::OnTaskError(int request_id, const URLFetcher* source,
                                      int error_code) {
  request_id_ = request_id;
  error_code_ = error_code;
  run_loop_.Quit();
}

bool QuicProxyServerTest::Initialize(int port) {
  std::string str_port = base::IntToString(port);

  // initialize http fetcher
  stellite::HttpRequestContextGetter::Params context_params;
  context_params.enable_quic = true;
  context_params.enable_http2 = true;
  context_params.origins_to_force_quic_on.push_back(
      "test.example.com:" + str_port);
  context_params.ignore_certificate_errors = true;

  scoped_refptr<stellite::HttpRequestContextGetter> context_getter =
          new stellite::HttpRequestContextGetter(
              context_params, base::ThreadTaskRunnerHandle::Get());
  http_fetcher_.reset(new stellite::HttpFetcher(context_getter));

  // set host resolver
  std::unique_ptr<MockHostResolver> resolver(new MockHostResolver());
  resolver->rules()->AddRule("test.example.com", "127.0.0.1");
  std::unique_ptr<MappedHostResolver> host_resolver(
      new MappedHostResolver(std::move(resolver)));
  std::string map_rule = "MAP test.example.com test.example.com:" + str_port;
  host_resolver->AddRuleFromString(map_rule);
  context_getter->set_host_resolver(std::move(host_resolver));

  // set cert verifier
  base::FilePath root_dir;
  base::PathService::Get(base::DIR_SOURCE_ROOT, &root_dir);
  base::FilePath cert_dir =
      root_dir.Append(FILE_PATH_LITERAL(kCertificatePath));

  std::unique_ptr<MockCertVerifier> cert_verifier(
      new MockCertVerifier());

  CertVerifyResult verify_result;
  verify_result.verified_cert = ImportCertFromFile(
      cert_dir, "quic_test.example.com.crt");

  cert_verifier->AddResultForCertAndHost(verify_result.verified_cert.get(),
                                         "test.example.com", verify_result,
                                         OK);

  verify_result.verified_cert = ImportCertFromFile(
      cert_dir, "quic_test_ecc.example.com.crt");
  cert_verifier->AddResultForCertAndHost(verify_result.verified_cert.get(),
                                         "test.example.com", verify_result,
                                         OK);
  context_getter->set_cert_verifier(std::move(cert_verifier));

  // setup embedd http server
  test_server_.reset(new EmbeddedTestServer);
  test_server_->AddDefaultHandlers(
      base::FilePath("net/data/url_fetcher_impl_unittest"));
  EXPECT_EQ(test_server_->Start(), true);

  // setup quic server
  std::unique_ptr<ProofSourceChromium> proof_source(new ProofSourceChromium());
  base::FilePath root_path;
  base::PathService::Get(base::DIR_SOURCE_ROOT, &root_path);

  proof_source->Initialize(
      cert_dir.Append(FILE_PATH_LITERAL("quic_test.example.com.crt")),
      cert_dir.Append(FILE_PATH_LITERAL("quic_test.example.com.key.pkcs8")),
      cert_dir.Append(FILE_PATH_LITERAL("quic_test.example.com.key.sct")));

  IPEndPoint bind_address(IPAddress::IPv4AllZeros(), port);

  server_config_.proxy_pass(
      "http://127.0.0.1:" +
      base::IntToString(test_server_->host_port_pair().port()));

  proxy_worker_.reset(new QuicProxyWorker(
          base::ThreadTaskRunnerHandle::Get(),
          base::ThreadTaskRunnerHandle::Get(),
          bind_address,
          quic_config_,
          server_config_,
          AllSupportedVersions(),
          std::move(proof_source)));

  proxy_worker_->Initialize();
  proxy_worker_->SetStrikeRegisterNoStartupPeriod();
  proxy_worker_->Start();

  return true;
}

}  // namespace anonymous

TEST_F(QuicProxyServerTest, TestGetRequest) {
  Initialize(6677);

  stellite::HttpRequest http_request;
  http_request.url = "https://test.example.com:6677";
  http_request.request_type = stellite::HttpRequest::GET;
  int request_id =
      http_fetcher_->Request(http_request, 0, weak_factory_.GetWeakPtr());

  run_loop_.Run();
  EXPECT_EQ(last_request_id(), request_id);
}

}  // namespace net
