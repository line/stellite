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

#ifndef STELLITE_CRYPTO_QUIC_SIGNATURE_CREATOR_H_
#define STELLITE_CRYPTO_QUIC_SIGNATURE_CREATOR_H_

#include "net/base/net_export.h"
#include "crypto/signature_creator.h"

namespace net {

class NET_EXPORT QuicSignatureCreator {
 public:
  ~QuicSignatureCreator();

  static QuicSignatureCreator* CreateRSAPSS(crypto::RSAPrivateKey* key);

  bool Update(const uint8* data_part, int data_part_len);

  bool Final(std::vector<uint8>* signature);

 private:
  // Private constructor
  QuicSignatureCreator();

#if defined(USE_OPENSSL)
  EVP_MD_CTX* sign_context_;
#elif defined(USE_NSS_CERTS) || defined(OS_WIN) || defined(OS_MACOSX)
  SGNContextStr* sign_context_;
#endif

  DISALLOW_COPY_AND_ASSIGN(QuicSignatureCreator);
};

} // namespace net

#endif // STELLITE_CRYPTO_QUIC_SIGNATURE_CREATOR_H_
