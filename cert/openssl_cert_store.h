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

#ifndef STELLITE_CERT_X509_OPENSSL_CERT_STORE_H_
#define STELLITE_CERT_X509_OPENSSL_CERT_STORE_H_

#include "net/ssl/scoped_openssl_types.h"
#include "base/memory/singleton.h"

namespace net {

// OpenSSLCertStore keeps CA certificate files to process a
// CertVerifyProc::VerifyInternal interface
class OpenSSLCertStore {
 public:
  static OpenSSLCertStore* GetInstance() {
    return base::Singleton<OpenSSLCertStore, base::LeakySingletonTraits<
        OpenSSLCertStore>>::get();
  }

  bool ResetCertStore();

  bool ResetCertStore(const std::string& ca_file);

  X509_STORE* store() {
    return store_.get();
  }

 private:
  friend struct base::DefaultSingletonTraits<OpenSSLCertStore>;

  OpenSSLCertStore();
  ~OpenSSLCertStore();

  crypto::ScopedOpenSSL<X509_STORE, X509_STORE_free> store_;

  DISALLOW_COPY_AND_ASSIGN(OpenSSLCertStore);
};


} // namespace net

#endif // STELLITE_CERT_X509_OPENSSL_CERT_STORE_H_
