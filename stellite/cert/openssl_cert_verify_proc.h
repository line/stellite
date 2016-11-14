//
// Copyright 2016 LINE Corporation
//

#ifndef STELLITE_CERT_CERT_VERIFY_PROC_PEM_H_
#define STELLITE_CERT_CERT_VERIFY_PROC_PEM_H_

#include "net/cert/cert_verify_proc.h"

namespace net {

class OpenSSLCertVerifyProc : public CertVerifyProc {
 public:
  OpenSSLCertVerifyProc();

  bool SupportsAdditionalTrustAnchors() const override;
  bool SupportsOCSPStapling() const override;

 protected:
  ~OpenSSLCertVerifyProc() override;

 private:
  int VerifyInternal(X509Certificate* cert,
                     const std::string& hostname,
                     const std::string& ocsp_response,
                     int flags,
                     CRLSet* crl_set,
                     const CertificateList& additional_trust_anchors,
                     CertVerifyResult* verify_result) override;
};

} // namespace net

#endif // STELLITE_CERT_CERT_VERIFY_PROC_PEM_H_