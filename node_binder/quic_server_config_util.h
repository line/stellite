//
// 2017 writen by snibug@gmail.com
//

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
