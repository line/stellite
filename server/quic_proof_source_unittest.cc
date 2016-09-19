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

#include <string>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/string_piece.h"
#include "crypto/ec_private_key.h"
#include "crypto/rsa_private_key.h"
#include "crypto/signature_verifier.h"
#include "stellite/crypto/quic_signature_creator.h"
#include "net/base/test_completion_callback.h"
#include "net/cert/asn1_util.h"
#include "net/cert/cert_policy_enforcer.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/pem_tokenizer.h"
#include "net/cert/test_root_certs.h"
#include "net/cert/x509_certificate.h"
#include "net/http/transport_security_state.h"
#include "net/quic/crypto/proof_source.h"
#include "net/quic/crypto/proof_verifier.h"
#include "net/quic/crypto/proof_verifier_chromium.h"
#include "testing/gtest/include/gtest/gtest.h"

scoped_refptr<net::X509Certificate> ImportCert(const std::string& cert_file) {
  base::FilePath test_path;
  PathService::Get(base::DIR_SOURCE_ROOT, &test_path);
  test_path = test_path.AppendASCII("stellite").
                        AppendASCII("test_tools");

  base::FilePath cert_path = test_path.AppendASCII(cert_file);
  std::string cert_data;
  EXPECT_TRUE(base::ReadFileToString(cert_path, &cert_data));

  net::CertificateList certs;

  certs = net::X509Certificate::CreateCertificateListFromBytes(
      cert_data.c_str(),
      cert_data.size(),
      net::X509Certificate::FORMAT_AUTO);
  if (certs.empty()) {
    return NULL;
  }

  net::X509Certificate::OSCertHandles intermediates;
  for (size_t i = 0; i < certs.size(); ++i) {
    intermediates.push_back(certs[i]->os_cert_handle());
  }

  scoped_refptr<net::X509Certificate>
      result(net::X509Certificate::CreateFromHandle(
          certs[0]->os_cert_handle(), intermediates));
  return result;
}

class QuicSourceProofTest : public testing::Test {
 public:
  QuicSourceProofTest() {
  }

  ~QuicSourceProofTest() override {}

  void SetUp() override {
    std::string cert_data;
    EXPECT_TRUE(ReadFromFile("leaf_cert.pem", &cert_data));

    certificate_list_ = net::X509Certificate::CreateCertificateListFromBytes(
        cert_data.c_str(),
        cert_data.size(),
        net::X509Certificate::FORMAT_AUTO);
    EXPECT_TRUE(certificate_list_.size());

    std::string key_data;
    EXPECT_TRUE(ReadFromFile("leaf_cert.pkcs8", &key_data));

    std::vector<std::string> block_headers;
    block_headers.push_back("RSA PRIVATE KEY");
    block_headers.push_back("EC PRIVATE KEY");
    block_headers.push_back("PRIVATE KEY");

    net::PEMTokenizer key_pem_tokenizer(key_data, block_headers);
    EXPECT_TRUE(key_pem_tokenizer.GetNext());

    const std::string& key_der = key_pem_tokenizer.data();
    std::vector<uint8> key_info(
        reinterpret_cast<const uint8*>(key_der.c_str()),
        reinterpret_cast<const uint8*>(key_der.c_str() + key_der.size()));

    rsa_private_key_.reset(crypto::RSAPrivateKey::CreateFromPrivateKeyInfo(
            key_info));

    EXPECT_TRUE(rsa_private_key_.get());
  }

  bool ReadFromFile(const std::string& file_name, std::string* out_stream) {
    base::FilePath test_path;
    PathService::Get(base::DIR_SOURCE_ROOT, &test_path);
    test_path = test_path.AppendASCII("stellite").
                          AppendASCII("test_tools");
    base::FilePath file_path = test_path.AppendASCII(file_name);
    return base::ReadFileToString(file_path, out_stream);
  }

  void TearDown() override {
  }

  std::unique_ptr<crypto::RSAPrivateKey> rsa_private_key_;
  std::unique_ptr<crypto::ECPrivateKey> ec_private_key_;
  net::CertificateList certificate_list_;
};

class TestQuicProofVerifierCallback : public net::ProofVerifierCallback {
 public:
  TestQuicProofVerifierCallback(net::TestCompletionCallback* comp_callback,
                                bool* ok,
                                std::string* error_details)
      : comp_callback_(comp_callback),
        ok_(ok),
        error_details_(error_details) {
  }

  void Run(bool ok,
           const std::string& error_details,
           std::unique_ptr<net::ProofVerifyDetails>* details) override {
    *ok_ = ok;
    *error_details_ = error_details;
    comp_callback_->callback().Run(0);
  }

 private:
  net::TestCompletionCallback* const comp_callback_;
  bool* const ok_;
  std::string* const error_details_;
};

class TestQuicProofVerifierChromium : public net::ProofVerifierChromium {
 public:
  TestQuicProofVerifierChromium(
      std::unique_ptr<net::CertVerifier> cert_verifier,
      net::CertPolicyEnforcer* cert_policy_enforcer,
      net::TransportSecurityState* transport_security_state,
      const std::string& cert_file)
      : ProofVerifierChromium(cert_verifier.get(), cert_policy_enforcer,
                              transport_security_state),
        cert_verifier_(cert_verifier.Pass()),
        transport_security_state_(transport_security_state) {
    scoped_refptr<net::X509Certificate> root_cert = ImportCert(cert_file);
    scoped_root_.Reset(root_cert.get());
  }

 private:
  net::ScopedTestRoot scoped_root_;
  std::unique_ptr<net::CertVerifier> cert_verifier_;
  std::unique_ptr<net::TransportSecurityState> transport_security_state_;
};

TEST_F(QuicSourceProofTest, VerifyCertificate) {
  std::unique_ptr<TestQuicProofVerifierChromium>
      verifier(new TestQuicProofVerifierChromium(
          net::CertVerifier::CreateDefault(),
          new net::CertPolicyEnforcer(),
          new net::TransportSecurityState,
          "leaf_cert.pem"));
}

TEST_F(QuicSourceProofTest, SignAndVerifyWithLineAppsBetaCertificate) {
  // Signing
  std::unique_ptr<net::QuicSignatureCreator> signer(
      net::QuicSignatureCreator::CreateRSAPSS(
          rsa_private_key_.get()));

  const std::string message = "hello world";
  signer->Update(reinterpret_cast<const uint8*>(message.c_str()),
                 message.size());

  std::vector<uint8> signature;
  EXPECT_TRUE(signer->Final(&signature));

  // Verify
  scoped_refptr<net::X509Certificate> cert =
      ImportCert("example.com.cert.pem");
  std::string der_cert;
  EXPECT_TRUE(net::X509Certificate::GetDEREncoded(
          cert->os_cert_handle(),
          &der_cert));

  base::StringPiece spki;
  EXPECT_TRUE(net::asn1::ExtractSPKIFromDERCert(der_cert,
                                                &spki));

  crypto::SignatureVerifier verifier;
  unsigned int hash_length = 32;
  EXPECT_TRUE(verifier.VerifyInitRSAPSS(
          crypto::SignatureVerifier::SHA256,
          crypto::SignatureVerifier::SHA256,
          hash_length,
          &signature.front(),
          signature.size(),
          reinterpret_cast<const uint8*>(spki.data()),
          spki.size()));

  verifier.VerifyUpdate(reinterpret_cast<const uint8*>(message.c_str()),
                        message.size());

  EXPECT_TRUE(verifier.VerifyFinal());
}

