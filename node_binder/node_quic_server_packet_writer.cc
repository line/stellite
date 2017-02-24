//
// 2017 write by snibug@gmail.com
//

#include "node_binder/node_quic_server_packet_writer.h"

#include "net/quic/core/quic_utils.h"
#include "net/base/io_buffer.h"
#include "node_binder/socket/udp_server.h"
#include "net/base/net_errors.h"

namespace stellite {

NodeQuicServerPacketWriter::NodeQuicServerPacketWriter(UDPServer* server)
    : server_(server),
      write_blocked_(false),
      weak_factory_(this) {
}

NodeQuicServerPacketWriter::~NodeQuicServerPacketWriter() {}

net::WriteResult NodeQuicServerPacketWriter::WritePacket(
    const char* buffer,
    size_t buffer_len,
    const net::IPAddress& self_address,
    const net::IPEndPoint& peer_address,
    net::PerPacketOptions* options) {
  DCHECK(!IsWriteBlocked());
  scoped_refptr<net::StringIOBuffer> buf(
      new net::StringIOBuffer(std::string(buffer, buffer_len)));

  int rv = 0;
  if (buffer_len <= static_cast<size_t>(std::numeric_limits<int>::max())) {
    rv = server_->SendTo(
        buf.get(), static_cast<int>(buffer_len), &peer_address,
        base::Bind(&NodeQuicServerPacketWriter::OnWriteComplete,
                   weak_factory_.GetWeakPtr()));
  } else {
    rv = net::ERR_MSG_TOO_BIG;
  }

  net::WriteStatus status = net::WRITE_STATUS_OK;
  if (rv < 0) {
    if (rv != net::ERR_IO_PENDING) {
      status = net::WRITE_STATUS_ERROR;
    } else {
      status = net::WRITE_STATUS_BLOCKED;
      write_blocked_ = true;
    }
  }

  return net::WriteResult(status, rv);
}

bool NodeQuicServerPacketWriter::IsWriteBlockedDataBuffered() const {
  return true;
}

bool NodeQuicServerPacketWriter::IsWriteBlocked() const {
  return write_blocked_;
}

void NodeQuicServerPacketWriter::SetWritable() {
  write_blocked_ = false;
}

net::QuicByteCount NodeQuicServerPacketWriter::GetMaxPacketSize(
    const net::IPEndPoint& peer_address) const {
  return net::kMaxPacketSize;
}

void NodeQuicServerPacketWriter::OnWriteComplete(int rv) {
  DCHECK_NE(rv, net::ERR_IO_PENDING);
  write_blocked_ = false;
}

}  // namespace stellite
