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

#include "stellite/crypto/quic_ephemeral_key_source.h"

#include "net/quic/core/crypto/key_exchange.h"
#include "net/quic/core/crypto/quic_random.h"

const int64_t kExpireSeconds = 60;

namespace net {

QuicEphemeralKeySource::QuicEphemeralKeySource()
    : calculate_time_(base::Time::Now()) {
}

QuicEphemeralKeySource::~QuicEphemeralKeySource() {}

std::string QuicEphemeralKeySource::CalculateForwardSecureKey(
    const KeyExchange* key_exchange,
    QuicRandom* random,
    QuicTime quic_now,
    base::StringPiece peer_public_value,
    std::string* public_value) {

  // quic server ephemeral key pair caching
  // This idea is referenced from devsister go-quic server
  // https://github.com/devsisters/goquic
  base::Time now(base::Time::Now());
  if ((now - calculate_time_).InSeconds() < kExpireSeconds &&
      server_keypair_.get() != nullptr) {
    return CalculateSharedKey(server_keypair_.get(), peer_public_value,
                              public_value);
  }

  calculate_time_ = std::move(now);
  server_keypair_.reset(key_exchange->NewKeyPair(random));
  return CalculateSharedKey(server_keypair_.get(), peer_public_value,
                            public_value);
}

std::string QuicEphemeralKeySource::CalculateSharedKey(
    KeyExchange* keypair,
    base::StringPiece peer_public_value,
    std::string* public_value) {

  std::string forward_secure_secret;
  keypair->CalculateSharedKey(peer_public_value,
                              &forward_secure_secret);

  public_value->assign(keypair->public_value().as_string());

  return forward_secure_secret;
}

} // namespace net
