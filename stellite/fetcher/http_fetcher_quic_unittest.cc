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

#include "base/memory/ref_counted.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/cert/mock_cert_verifier.h"
#include "net/dns/mapped_host_resolver.h"
#include "net/dns/mock_host_resolver.h"
#include "net/quic/chromium/crypto/proof_source_chromium.h"
#include "net/test/cert_test_util.h"
#include "net/tools/quic/quic_simple_server.h"
#include "stellite/fetcher/http_fetcher.h"
#include "stellite/fetcher/http_request_context_getter.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace stellite {

namespace {
const char* kCertificatePath = "src/net/data/ssl/certificates";

class HttpFetcherQuicTest : public testing::Test,
                            public HttpFetcherTask::Visitor {
 public:
  HttpFetcherQuicTest();
  ~HttpFetcherQuicTest() override;

  void Initialize(int port);

  void OnTaskComplete(int request_id, const net::URLFetcher* source,
                      const net::HttpResponseInfo* response) override;

  void OnTaskStream(int request_id, const net::URLFetcher* source,
                    const net::HttpResponseInfo* response,
                    const char* data, size_t len, bool fin) override;

  void OnTaskError(int request_id, const net::URLFetcher* source,
                   int error_code) override;

  int last_request_id() {
    return request_id_;
  }

  int last_error_code() {
    return error_code_;
  }

  std::unique_ptr<net::QuicSimpleServer> quic_server_;
  std::unique_ptr<HttpFetcher> http_fetcher_;

  int request_id_;
  int error_code_;

  base::WeakPtrFactory<HttpFetcherQuicTest> weak_factory_;
};

HttpFetcherQuicTest::HttpFetcherQuicTest()
    : request_id_(0),
      error_code_(0),
      weak_factory_(this) {
}

HttpFetcherQuicTest::~HttpFetcherQuicTest() {}

void HttpFetcherQuicTest::Initialize(int port) {
  // initialize http fetcher
  HttpRequestContextGetter::Params context_params;
  context_params.enable_quic = true;
  context_params.enable_http2 = true;
  context_params.origins_to_force_quic_on.push_back(
      "test.example.com:" + base::IntToString(port));
  context_params.ignore_certificate_errors = true;

  scoped_refptr<HttpRequestContextGetter> context_getter =
          new HttpRequestContextGetter(
              context_params, base::ThreadTaskRunnerHandle::Get());
  http_fetcher_.reset(new HttpFetcher(context_getter));

  // set host resolver
  std::unique_ptr<net::MockHostResolver> resolver(new net::MockHostResolver());
  resolver->rules()->AddRule("test.example.com", "127.0.0.1");
  std::unique_ptr<net::MappedHostResolver> host_resolver(
      new net::MappedHostResolver(std::move(resolver)));
  std::string map_rule = "MAP test.example.com test.example.com:" +
                         base::IntToString(port);
  host_resolver->AddRuleFromString(map_rule);
  context_getter->set_host_resolver(std::move(host_resolver));

  // set cert verifier
  base::FilePath root_dir;
  base::PathService::Get(base::DIR_SOURCE_ROOT, &root_dir);
  base::FilePath cert_dir =
      root_dir.Append(FILE_PATH_LITERAL(kCertificatePath));

  std::unique_ptr<net::MockCertVerifier> cert_verifier(
      new net::MockCertVerifier());

  net::CertVerifyResult verify_result;
  verify_result.verified_cert = net::ImportCertFromFile(
      cert_dir, "quic_test.example.com.crt");

  cert_verifier->AddResultForCertAndHost(verify_result.verified_cert.get(),
                                         "test.example.com", verify_result,
                                         net::OK);

  verify_result.verified_cert = net::ImportCertFromFile(
      cert_dir, "quic_test_ecc.example.com.crt");
  cert_verifier->AddResultForCertAndHost(verify_result.verified_cert.get(),
                                         "test.example.com", verify_result,
                                         net::OK);
  context_getter->set_cert_verifier(std::move(cert_verifier));

  // initialize quic server
  std::unique_ptr<net::ProofSourceChromium> proof_source(
      new net::ProofSourceChromium());
  proof_source->Initialize(
      cert_dir.Append(FILE_PATH_LITERAL("quic_test.example.com.crt")),
      cert_dir.Append(FILE_PATH_LITERAL("quic_test.example.com.key.pkcs8")),
      cert_dir.Append(FILE_PATH_LITERAL("quic_test.example.com.key.sct")));

  net::QuicConfig quic_config;
  quic_server_.reset(new net::QuicSimpleServer(
          std::move(proof_source), quic_config,
          net::QuicCryptoServerConfig::ConfigOptions(),
          net::AllSupportedVersions()));

  net::IPEndPoint bind_address(net::IPAddress::IPv4AllZeros(), port);
  int rv = quic_server_->Listen(bind_address);
  EXPECT_GE(rv, 0) << "quic server fails to listen";
}

void HttpFetcherQuicTest::OnTaskComplete(
    int request_id, const net::URLFetcher* source,
    const net::HttpResponseInfo* response) {
  request_id_ = request_id;
}

void HttpFetcherQuicTest::OnTaskStream(
    int request_id, const net::URLFetcher* source,
    const net::HttpResponseInfo* response,
    const char* data, size_t len, bool fin) {
  request_id_ = request_id;
}

void HttpFetcherQuicTest::OnTaskError(
    int request_id, const net::URLFetcher* source, int error_code) {
  request_id_ = request_id;
  error_code_ = error_code;
}

} // namespace anonymous

TEST_F(HttpFetcherQuicTest, TestGetRequest) {
  Initialize(9999);
  HttpRequest http_request;
  http_request.url = "https://test.example.com:9999";
  int request_id = http_fetcher_->Request(
      http_request, 1000, weak_factory_.GetWeakPtr());
  base::RunLoop().RunUntilIdle();
  CHECK_EQ(last_request_id(), request_id);
  CHECK_EQ(last_error_code(), 0);
}

} // namespace stellite
