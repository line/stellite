// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stellite/cert/openssl_cert_verify_proc.h"

#include <openssl/x509v3.h>

#include <string>
#include <vector>

#include "base/logging.h"
#include "base/sha1.h"
#include "crypto/openssl_util.h"
#include "crypto/scoped_openssl_types.h"
#include "crypto/sha2.h"
#include "stellite/cert/openssl_cert_store.h"
#include "net/base/net_errors.h"
#include "net/cert/asn1_util.h"
#include "net/cert/cert_status_flags.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/cert_verify_result.h"
#include "net/cert/test_root_certs.h"
#include "net/cert/x509_certificate.h"

namespace net {

namespace {

// Maps X509_STORE_CTX_get_error() return values to our cert status flags.
CertStatus MapCertErrorToCertStatus(int err) {
  switch (err) {
    case X509_V_ERR_SUBJECT_ISSUER_MISMATCH:
      return CERT_STATUS_COMMON_NAME_INVALID;
    case X509_V_ERR_CERT_NOT_YET_VALID:
    case X509_V_ERR_CERT_HAS_EXPIRED:
    case X509_V_ERR_CRL_NOT_YET_VALID:
    case X509_V_ERR_CRL_HAS_EXPIRED:
    case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
    case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
    case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD:
    case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD:
      return CERT_STATUS_DATE_INVALID;
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
    case X509_V_ERR_UNABLE_TO_GET_CRL:
    case X509_V_ERR_INVALID_CA:
    case X509_V_ERR_UNABLE_TO_GET_CRL_ISSUER:
    case X509_V_ERR_INVALID_NON_CA:
    case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
    case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
      return CERT_STATUS_AUTHORITY_INVALID;
#if 0
// TODO(bulach): what should we map to these status?
      return CERT_STATUS_NO_REVOCATION_MECHANISM;
      return CERT_STATUS_UNABLE_TO_CHECK_REVOCATION;
#endif
    case X509_V_ERR_CERT_REVOKED:
      return CERT_STATUS_REVOKED;
    // All these status are mapped to CERT_STATUS_INVALID.
    case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
    case X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE:
    case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
    case X509_V_ERR_CERT_SIGNATURE_FAILURE:
    case X509_V_ERR_CRL_SIGNATURE_FAILURE:
    case X509_V_ERR_OUT_OF_MEM:
    case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
    case X509_V_ERR_CERT_CHAIN_TOO_LONG:
    case X509_V_ERR_PATH_LENGTH_EXCEEDED:
    case X509_V_ERR_INVALID_PURPOSE:
    case X509_V_ERR_CERT_UNTRUSTED:
    case X509_V_ERR_CERT_REJECTED:
    case X509_V_ERR_AKID_SKID_MISMATCH:
    case X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH:
    case X509_V_ERR_UNHANDLED_CRITICAL_EXTENSION:
    case X509_V_ERR_KEYUSAGE_NO_CERTSIGN:
    case X509_V_ERR_KEYUSAGE_NO_CRL_SIGN:
    case X509_V_ERR_UNHANDLED_CRITICAL_CRL_EXTENSION:
    case X509_V_ERR_PROXY_PATH_LENGTH_EXCEEDED:
    case X509_V_ERR_KEYUSAGE_NO_DIGITAL_SIGNATURE:
    case X509_V_ERR_PROXY_CERTIFICATES_NOT_ALLOWED:
    case X509_V_ERR_INVALID_EXTENSION:
    case X509_V_ERR_INVALID_POLICY_EXTENSION:
    case X509_V_ERR_NO_EXPLICIT_POLICY:
    case X509_V_ERR_UNNESTED_RESOURCE:
    case X509_V_ERR_APPLICATION_VERIFICATION:
      return CERT_STATUS_INVALID;
    default:
      NOTREACHED() << "Invalid X509 err " << err;
      return CERT_STATUS_INVALID;
  }
}

// sk_X509_free is a function-style macro, so can't be used as a template
// param directly.
void sk_X509_free_fn(STACK_OF(X509)* st) {
  sk_X509_free(st);
}

void GetCertChainInfo(X509_STORE_CTX* store_ctx,
                      CertVerifyResult* verify_result) {
  STACK_OF(X509)* chain = X509_STORE_CTX_get_chain(store_ctx);
  std::vector<X509*> verified_chain;
  for (size_t i = 0; i < sk_X509_num(chain); ++i) {
    X509* cert = sk_X509_value(chain, i);
    if (i > 0) {
      verified_chain.push_back(cert);
    }

  // Only checks the algorithm status for certificates that are not in the
    // TrustStore.
    if (i < static_cast<size_t>(store_ctx->last_untrusted)) {
      int sig_alg = OBJ_obj2nid(cert->sig_alg->algorithm);
      if (sig_alg == NID_md2WithRSAEncryption) {
        verify_result->has_md2 = true;
      } else if (sig_alg == NID_md4WithRSAEncryption) {
        verify_result->has_md4 = true;
      } else if (sig_alg == NID_md5WithRSAEncryption ||
                 sig_alg == NID_md5WithRSA) {
        verify_result->has_md5 = true;
      } else if (sig_alg == NID_sha1WithRSAEncryption ||
                 sig_alg == NID_dsaWithSHA || sig_alg == NID_dsaWithSHA1 ||
                 sig_alg == NID_dsaWithSHA1_2 || sig_alg == NID_sha1WithRSA ||
                 sig_alg == NID_ecdsa_with_SHA1) {
        verify_result->has_sha1 = true;
      }
    }
  }
}

void AppendPublicKeyHashes(X509_STORE_CTX* store_ctx,
                           HashValueVector* hashes) {
  STACK_OF(X509)* chain = X509_STORE_CTX_get_chain(store_ctx);
  for (size_t i = 0; i < sk_X509_num(chain); ++i) {
    X509* cert = sk_X509_value(chain, i);

    std::string der_data;
    if (!X509Certificate::GetDEREncoded(cert, &der_data))
      continue;

    base::StringPiece der_bytes(der_data);
    base::StringPiece spki_bytes;
    if (!asn1::ExtractSPKIFromDERCert(der_bytes, &spki_bytes))
      continue;

    HashValue sha1(HASH_VALUE_SHA1);
    base::SHA1HashBytes(reinterpret_cast<const uint8_t*>(spki_bytes.data()),
                        spki_bytes.size(), sha1.data());
    hashes->push_back(sha1);

    HashValue sha256(HASH_VALUE_SHA256);
    crypto::SHA256HashString(spki_bytes, sha256.data(), crypto::kSHA256Length);
    hashes->push_back(sha256);
  }
}

} // namespace anonymous

OpenSSLCertVerifyProc::OpenSSLCertVerifyProc() {
}

bool OpenSSLCertVerifyProc::SupportsAdditionalTrustAnchors() const {
  return false;
}

bool OpenSSLCertVerifyProc::SupportsOCSPStapling() const {
  return false;
}

OpenSSLCertVerifyProc::~OpenSSLCertVerifyProc()  {
}

int OpenSSLCertVerifyProc::VerifyInternal(X509Certificate* cert,
    const std::string& hostname,
    const std::string& ocsp_response,
    int flags,
    CRLSet* crl_set,
    const CertificateList& additional_trust_anchors,
    CertVerifyResult* verify_result) {
  crypto::EnsureOpenSSLInit();

  if (!cert->VerifyNameMatch(hostname,
                             &verify_result->common_name_fallback_used)) {
    verify_result->cert_status |= CERT_STATUS_COMMON_NAME_INVALID;
  }

  crypto::ScopedOpenSSL<X509_STORE_CTX, X509_STORE_CTX_free> ctx(
      X509_STORE_CTX_new());

  crypto::ScopedOpenSSL<STACK_OF(X509), sk_X509_free_fn> intermediates(
      sk_X509_new_null());
  if (!intermediates.get())
    return ERR_OUT_OF_MEMORY;

  const X509Certificate::OSCertHandles& os_intermediates =
      cert->GetIntermediateCertificates();
  for (X509Certificate::OSCertHandles::const_iterator it =
       os_intermediates.begin(); it != os_intermediates.end(); ++it) {
    if (!sk_X509_push(intermediates.get(), *it))
      return ERR_OUT_OF_MEMORY;
  }
  if (X509_STORE_CTX_init(ctx.get(),
                          OpenSSLCertStore::GetInstance()->store(),
                          cert->os_cert_handle(), intermediates.get()) != 1) {
    NOTREACHED();
    return ERR_FAILED;
  }

  if (X509_verify_cert(ctx.get()) != 1) {
    int x509_error = X509_STORE_CTX_get_error(ctx.get());
    CertStatus cert_status = MapCertErrorToCertStatus(x509_error);
    LOG(ERROR) << "X509 Verification error "
        << X509_verify_cert_error_string(x509_error)
        << " : " << x509_error
        << " : " << X509_STORE_CTX_get_error_depth(ctx.get())
        << " : " << cert_status;
    verify_result->cert_status |= cert_status;
  }

  GetCertChainInfo(ctx.get(), verify_result);
  AppendPublicKeyHashes(ctx.get(), &verify_result->public_key_hashes);
  if (IsCertStatusError(verify_result->cert_status))
    return MapCertStatusToNetError(verify_result->cert_status);

  return OK;
}

} // namespace net