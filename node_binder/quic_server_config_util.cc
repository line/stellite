//
// 2017 writen by snibug@gmail.com
//

#include "node_binder/quic_server_config_util.h"

#include <memory>
#include <sstream>

#include "base/strings/string_number_conversions.h"
#include "net/quic/core/crypto/crypto_server_config_protobuf.h"
#include "net/quic/core/crypto/quic_crypto_server_config.h"
#include "net/quic/core/crypto/quic_random.h"
#include "net/quic/core/quic_clock.h"

namespace stellite {

QuicServerConfigUtil::QuicServerConfigUtil() {}
QuicServerConfigUtil::~QuicServerConfigUtil() {}

// static
net::QuicServerConfigProtobuf* QuicServerConfigUtil::GenerateConfig(
    bool use_p256,
    bool use_channel_id,
    uint32_t expire_seconds) {
  net::QuicClock quic_clock;
  net::QuicCryptoServerConfig::ConfigOptions config_options;

  const net::QuicWallTime now = quic_clock.WallNow();
  config_options.expiry_time = now.Add(
      net::QuicTime::Delta::FromSeconds(expire_seconds));

  // use p256 curve key exchange
  config_options.p256 = use_p256;

  // use id channel
  config_options.channel_id_enabled = use_channel_id;

  return net::QuicCryptoServerConfig::GenerateConfig(
      net::QuicRandom::GetInstance(),
      &quic_clock,
      config_options);
}

// static
net::QuicServerConfigProtobuf* QuicServerConfigUtil::DecodeConfig(
    const std::string& hex_encoded) {
  std::unique_ptr<net::QuicServerConfigProtobuf> protobuf(
      new net::QuicServerConfigProtobuf());

  std::vector<uint8_t> serialized;
  if (!base::HexStringToBytes(hex_encoded, &serialized)) {
    return nullptr;
  }

  uint32_t offset = 0;
  uint32_t config_length = -1;
  if (offset + sizeof(config_length) > serialized.size()) {
    return nullptr;
  }

  memcpy(&config_length, &serialized[offset], sizeof(config_length));
  offset += sizeof(config_length);

  if (offset + config_length > serialized.size()) {
    return nullptr;
  }

  std::string quic_server_config;
  quic_server_config.resize(config_length);
  memcpy(&quic_server_config[0], &serialized[offset], config_length);
  offset += config_length;

  protobuf->set_config(quic_server_config);

  while (offset + sizeof(net::QuicTag) < serialized.size()) {
    net::QuicTag tag;
    memcpy(&tag, &serialized[offset], sizeof(net::QuicTag));
    offset += sizeof(net::QuicTag);

    uint32_t private_key_length = 0;
    DCHECK(offset + sizeof(private_key_length) <= serialized.size());

    memcpy(&private_key_length, &serialized[offset],
           sizeof(private_key_length));

    offset += sizeof(private_key_length);
    DCHECK(offset + private_key_length <= serialized.size());

    std::string private_key;
    private_key.resize(private_key_length);
    memcpy(&private_key[0], &serialized[offset], private_key_length);
    offset += private_key_length;

    net::QuicServerConfigProtobuf::PrivateKey* key = protobuf->add_key();
    key->set_tag(tag);
    key->set_private_key(private_key);
  }

  return protobuf.release();
}

// static
bool QuicServerConfigUtil::EncodeConfig(
    const net::QuicServerConfigProtobuf* config,
    std::string* out) {

  if (!config || !out) {
    return false;
  }

  std::stringstream stream;

  // serialize a quic server config
  std::string quic_server_config = config->config();
  uint32_t config_length = quic_server_config.size();
  stream.write(reinterpret_cast<char*>(&config_length), sizeof(config_length));
  stream.write(quic_server_config.data(), config_length);

  for (size_t i = 0; i < config->key_size(); ++i) {
    const net::QuicServerConfigProtobuf::PrivateKey& key = config->key(i);
    net::QuicTag tag = key.tag();
    std::string private_key = key.private_key();
    uint32_t private_key_length = private_key.size();

    stream.write(reinterpret_cast<char*>(&tag), sizeof(tag));
    stream.write(reinterpret_cast<char*>(&private_key_length),
                 sizeof(private_key_length));
    stream.write(private_key.data(), private_key_length);
  }

  std::string serialized = stream.str();
  std::string hex_encoded = base::HexEncode(serialized.data(),
                                            serialized.size());
  out->assign(hex_encoded);
  return true;
}

}  // namespace stellte
