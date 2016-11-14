// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STELLITE_SOCKET_UDP_SERVER_SOCKET_H_
#define STELLITE_SOCKET_UDP_SERVER_SOCKET_H_

#include <stdint.h>

#include "base/macros.h"
#include "net/base/completion_callback.h"
#include "net/base/net_export.h"
#include "net/log/net_log_with_source.h"
#include "net/udp/datagram_server_socket.h"
#include "stellite/socket/quic_udp_socket.h"

namespace net {

class IPAddress;
class IPEndPoint;
struct NetLogSource;

// A client socket that uses UDP as the transport layer.
class NET_EXPORT QuicUDPServerSocket : public DatagramServerSocket {
 public:
  QuicUDPServerSocket();
  ~QuicUDPServerSocket() override;

  // Implement DatagramServerSocket:
  int Listen(const IPEndPoint& address) override;
  int RecvFrom(IOBuffer* buf,
               int buf_len,
               IPEndPoint* address,
               const CompletionCallback& callback) override;
  int SendTo(IOBuffer* buf,
             int buf_len,
             const IPEndPoint& address,
             const CompletionCallback& callback) override;
  const NetLogWithSource& NetLog() const override;
  int GetLocalAddress(IPEndPoint* address) const override;
  int GetPeerAddress(IPEndPoint* address) const override;
  int JoinGroup(const IPAddress& group_address) const override;
  int LeaveGroup(const IPAddress& group_address) const override;
  int SetDiffServCodePoint(DiffServCodePoint dscp) override;
  int SetDoNotFragment() override;
  int SetMulticastInterface(uint32_t interface_index) override;
  int SetMulticastLoopbackMode(bool loopback) override;
  int SetMulticastTimeToLive(int time_to_live) override;
  int SetReceiveBufferSize(int32_t size) override;
  int SetSendBufferSize(int32_t size) override;
  void AllowAddressReuse() override;
  void AllowBroadcast() override;
  void AllowPortReuse();
  void Close() override;
  void DetachFromThread() override;
  void UseNonBlockingIO() override;

 private:
  QuicUDPSocket socket_;
  NetLogWithSource net_log_;

  bool allow_address_reuse_;
  bool allow_port_reuse_;
  bool allow_broadcast_;
  DISALLOW_COPY_AND_ASSIGN(QuicUDPServerSocket);
};

}  // namespace net

#endif  // STELLITE_SOCKET_UDP_SERVER_SOCKET_H_
