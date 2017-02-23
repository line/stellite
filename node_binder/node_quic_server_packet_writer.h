//
// 2017 writen by snibug@gmail.com
//

#ifndef NODE_BINDER_NODE_QUIC_SERVER_PACKET_WRITER_H_
#define NODE_BINDER_NODE_QUIC_SERVER_PACKET_WRITER_H_

#include "base/memory/weak_ptr.h"
#include "net/quic/core/quic_packet_writer.h"
#include "stellite/include/stellite_export.h"

namespace stellite {

class UDPServer;

class STELLITE_EXPORT NodeQuicServerPacketWriter
    : public net::QuicPacketWriter {
 public:
  NodeQuicServerPacketWriter(UDPServer* udp_server);
  ~NodeQuicServerPacketWriter() override;

  net::WriteResult WritePacket(const char* buffer,
                          size_t buffer_len,
                          const net::IPAddress& self_address,
                          const net::IPEndPoint& peer_address,
                          net::PerPacketOptions* options) override;

  // QuicPacketWriter implements
  bool IsWriteBlockedDataBuffered() const override;
  bool IsWriteBlocked() const override;
  void SetWritable() override;
  net::QuicByteCount GetMaxPacketSize(
      const net::IPEndPoint& peer_address) const override;
  void OnWriteComplete(int result);

 private:
  UDPServer* server_;
  bool write_blocked_;

  base::WeakPtrFactory<NodeQuicServerPacketWriter> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(NodeQuicServerPacketWriter);
};

}  // namespace stellite

#endif  // NODE_BINDER_NODE_QUIC_SERVER_PACKET_WRITER_H_
