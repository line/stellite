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

#include "stellite/cert/openssl_cert_store.h"

#include <openssl/x509_vfy.h>

#include "crypto/openssl_util.h"

namespace net {

OpenSSLCertStore::OpenSSLCertStore() {
  crypto::EnsureOpenSSLInit();
  ResetCertStore();
}

OpenSSLCertStore::~OpenSSLCertStore() {}

bool OpenSSLCertStore::ResetCertStore(const std::string& ca_file) {
  store_.reset(X509_STORE_new());
  DCHECK(store_.get());
  return X509_STORE_load_locations(store_.get(), ca_file.c_str(), NULL) == 1;
}

bool OpenSSLCertStore::ResetCertStore() {
  store_.reset(X509_STORE_new());
  DCHECK(store_.get());
  return X509_STORE_set_default_paths(store_.get()) == 1;
}

} // namespace net
