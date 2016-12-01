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

#ifndef STELLITE_CRYPTO_QUIC_EPHEMERAL_KEY_SOURCE
#define STELLITE_CRYPTO_QUIC_EPHEMERAL_KEY_SOURCE

#include <memory>

#include "base/time/time.h"
#include "net/quic/core/crypto/ephemeral_key_source.h"

namespace net {
class KeyExchange;

class QuicEphemeralKeySource: public EphemeralKeySource {
 public:
  QuicEphemeralKeySource();
  ~QuicEphemeralKeySource() override;

  std::string CalculateForwardSecureKey(
      const KeyExchange* key_exchange,
      QuicRandom* rand,
      QuicTime now,
      base::StringPiece peer_public_value,
      std::string* public_value) override;

 private:
  std::string CalculateSharedKey(KeyExchange* keypair,
                                 base::StringPiece peer_public_value,
                                 std::string* public_value);

  base::Time calculate_time_;
  std::unique_ptr<KeyExchange> server_keypair_;

  DISALLOW_COPY_AND_ASSIGN(QuicEphemeralKeySource);
};

}  // namespacea net

#endif // STELLITE_CRYPTO_QUIC_EPHEMERAL_KEY_SOURCE
