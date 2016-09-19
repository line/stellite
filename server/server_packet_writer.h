// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STELLITE_SERVER_SERVER_PACKET_WRITER_H_
#define STELLITE_SERVER_SERVER_PACKET_WRITER_H_

#include <stddef.h>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "stellite/socket/quic_udp_server_socket.h"
#include "net/base/ip_endpoint.h"
#include "net/quic/quic_connection.h"
#include "net/quic/quic_packet_writer.h"
#include "net/quic/quic_protocol.h"

namespace net {

class IPAddress;
class QuicBlockedWriterInterface;
class UDPServerSocket;
struct WriteResult;


// Chrome specific packet writer which uses a UDPServerSocket for writing
// data.
class NET_EXPORT ServerPacketWriter : public QuicPacketWriter {
 public:
  typedef base::Callback<void(WriteResult)> WriteCallback;

  ServerPacketWriter(QuicUDPServerSocket* socket,
                     QuicBlockedWriterInterface* blocked_writer);
  ~ServerPacketWriter() override;

  // Use this method to write packets rather than WritePacket:
  // ServerPacketWriter requires a callback to exist for every
  // write, which will be called once the write is complete.
  virtual WriteResult WritePacketWithCallback(const char* buffer,
                                              size_t buf_len,
                                              const IPAddress& self_address,
                                              const IPEndPoint& peer_address,
                                              PerPacketOptions* options,
                                              WriteCallback callback);

  void OnWriteComplete(int rv);

  // QuicPacketWriter implementation:
  bool IsWriteBlockedDataBuffered() const override;
  bool IsWriteBlocked() const override;
  void SetWritable() override;
  QuicByteCount GetMaxPacketSize(const IPEndPoint& peer_address) const override;

 protected:
  // Do not call WritePacket on its own -- use WritePacketWithCallback
  WriteResult WritePacket(const char* buffer,
                          size_t buf_len,
                          const IPAddress& self_address,
                          const IPEndPoint& peer_address,
                          PerPacketOptions* options) override;

 private:
  QuicUDPServerSocket* socket_;

  // To be notified after every successful asynchronous write.
  QuicBlockedWriterInterface* blocked_writer_;

  // To call once the write is complete.
  WriteCallback callback_;

  // Whether a write is currently in-flight.
  bool write_blocked_;

  base::WeakPtrFactory<ServerPacketWriter> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ServerPacketWriter);
};

}  // namespace net

#endif  // STELLITE_SERVER_SERVER_PACKET_WRITER_H_