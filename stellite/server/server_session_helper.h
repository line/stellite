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

#ifndef STELLITE_SERVER_SERVER_SESSION_HELPER_H_
#define STELLITE_SERVER_SERVER_SESSION_HELPER_H_

#include "net/quic/core/quic_server_session_base.h"

namespace net {
class QuicRandom;

class ServerSessionHelper : public QuicCryptoServerStream::Helper {
 public:
  ServerSessionHelper(QuicRandom* random);
  ~ServerSessionHelper() override;

  QuicConnectionId GenerateConnectionIdForReject(
      QuicConnectionId connection_id) const override;

  bool CanAcceptClientHello(const CryptoHandshakeMessage& message,
                            const IPEndPoint& self_address,
                            std::string* error_details) const override;

 private:
  QuicRandom* random_;

  DISALLOW_COPY_AND_ASSIGN(ServerSessionHelper);
};

}  // namespace net

#endif  // STELLITE_SERVER_SERVER_SESSION_HELPER_H_
