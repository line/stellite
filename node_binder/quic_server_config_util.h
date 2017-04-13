// Copyright 2017 LINE Corporation
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

#ifndef NODE_BINDER_QUIC_SERVER_CONFIG_UTIL_H_
#define NODE_BINDER_QUIC_SERVER_CONFIG_UTIL_H_

#include <string>

#include "base/macros.h"

namespace net {

class QuicServerConfigProtobuf;

}  // namespace net

namespace stellite {

class QuicServerConfigUtil {
 public:
  // generate new Config
  static net::QuicServerConfigProtobuf* GenerateConfig(
      bool use_p256,
      bool use_channel_id,
      uint32_t expire_seconds);

// encode a CryptoServerConfigProtobuf
  static bool EncodeConfig(const net::QuicServerConfigProtobuf* config,
                           std::string* out);

// decode a serialized a quic server config
  static net::QuicServerConfigProtobuf* DecodeConfig(
      const std::string& hex_encoded);

 private:
  QuicServerConfigUtil();
  ~QuicServerConfigUtil();

  DISALLOW_COPY_AND_ASSIGN(QuicServerConfigUtil);
};

}  // namespace stellite

#endif  // NODE_BINDER_QUIC_SERVER_CONFIG_UTIL_H_
