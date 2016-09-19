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

#include "stellite/crypto/quic_signature_creator.h"

#include <openssl/evp.h>
#include <openssl/rsa.h>

#include <memory>

#include "base/logging.h"
#include "base/stl_util.h"
#include "crypto/openssl_util.h"
#include "crypto/rsa_private_key.h"
#include "crypto/scoped_openssl_types.h"
#include "stellite/logging/logging.h"

namespace net {

QuicSignatureCreator::QuicSignatureCreator()
    : sign_context_(EVP_MD_CTX_create()) {
}

QuicSignatureCreator::~QuicSignatureCreator() {
  EVP_MD_CTX_destroy(sign_context_);
}

// Static
QuicSignatureCreator* QuicSignatureCreator::CreateRSAPSS(
    crypto::RSAPrivateKey* key) {
  crypto::OpenSSLErrStackTracer error_tracer(FROM_HERE);

  std::unique_ptr<QuicSignatureCreator> result(new QuicSignatureCreator());
  const EVP_MD* const digest = EVP_sha256();
  if (!digest) {
    FLOG(ERROR) << "EVP_sha256 failed";
    return NULL;
  }

  EVP_PKEY_CTX* pkey_ctx;
  if (!EVP_DigestSignInit(result->sign_context_, &pkey_ctx, digest, NULL,
                          key->key())) {
    FLOG(ERROR) << "EVP_DigestSignInit failed";
    return NULL;
  }

  if (EVP_PKEY_CTX_set_rsa_padding(pkey_ctx, RSA_PKCS1_PSS_PADDING) != 1) {
    FLOG(ERROR) << "EVP_PKEY_CTX_set_rsa_padding failed";
    return NULL;
  }

  const EVP_MD* const mgf_digest = EVP_sha256();
  if (EVP_PKEY_CTX_set_rsa_mgf1_md(pkey_ctx, mgf_digest) != 1) {
    FLOG(ERROR) << "EVP_PKEY_CTX_set_rsa_mgf1_md failed";
    return NULL;
  }

  int salt_len = 32; // SHA256 hash length
  if (EVP_PKEY_CTX_set_rsa_pss_saltlen(pkey_ctx, salt_len) != 1) {
    FLOG(ERROR) << "EVP_PKEY_CTX_set_rsa_pss_saltlen failed";
    return NULL;
  }

  return result.release();
}

bool QuicSignatureCreator::Update(const uint8* data_part, int data_part_len) {
  crypto::OpenSSLErrStackTracer err_tracer(FROM_HERE);
  return !!EVP_DigestSignUpdate(sign_context_, data_part, data_part_len);
}

bool QuicSignatureCreator::Final(std::vector<uint8>* signature) {
  crypto::OpenSSLErrStackTracer err_tracer(FROM_HERE);

  // Determine the maximum length of the signature.
  size_t len = 0;
  if (!EVP_DigestSignFinal(sign_context_, NULL, &len)) {
    signature->clear();
    return false;
  }
  signature->resize(len);

  // Sign it.
  if (!EVP_DigestSignFinal(sign_context_, signature->data(), &len)) {
    signature->clear();
    return false;
  }
  signature->resize(len);
  return true;
}

}  // namespace net
