// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stellite/socket/quic_udp_server_socket.h"

#include "net/base/net_errors.h"
#include "net/base/rand_callback.h"

namespace net {

QuicUDPServerSocket::QuicUDPServerSocket()
    : socket_(DatagramSocket::DEFAULT_BIND, RandIntCallback()),
      allow_address_reuse_(false),
      allow_port_reuse_(false),
      allow_broadcast_(false) {}

QuicUDPServerSocket::~QuicUDPServerSocket() {
}

int QuicUDPServerSocket::Listen(const IPEndPoint& address) {
  int rv = socket_.Open(address.GetFamily());
  if (rv != OK)
    return rv;

  if (allow_address_reuse_) {
    rv = socket_.AllowAddressReuse();
    if (rv != OK) {
      socket_.Close();
      return rv;
    }
  }

  if (allow_port_reuse_) {
    rv = socket_.AllowPortReuse();
    if (rv != OK) {
      socket_.Close();
      return rv;
    }
  }

  if (allow_broadcast_) {
    rv = socket_.SetBroadcast(true);
    if (rv != OK) {
      socket_.Close();
      return rv;
    }
  }

  return socket_.Bind(address);
}

int QuicUDPServerSocket::RecvFrom(IOBuffer* buf,
                              int buf_len,
                              IPEndPoint* address,
                              const CompletionCallback& callback) {
  return socket_.RecvFrom(buf, buf_len, address, callback);
}

int QuicUDPServerSocket::SendTo(IOBuffer* buf,
                            int buf_len,
                            const IPEndPoint& address,
                            const CompletionCallback& callback) {
  return socket_.SendTo(buf, buf_len, address, callback);
}

int QuicUDPServerSocket::SetReceiveBufferSize(int32_t size) {
  return socket_.SetReceiveBufferSize(size);
}

int QuicUDPServerSocket::SetSendBufferSize(int32_t size) {
  return socket_.SetSendBufferSize(size);
}

int QuicUDPServerSocket::SetDoNotFragment() {
  return socket_.SetDoNotFragment();
}

void QuicUDPServerSocket::Close() {
  socket_.Close();
}

int QuicUDPServerSocket::GetPeerAddress(IPEndPoint* address) const {
  return socket_.GetPeerAddress(address);
}

int QuicUDPServerSocket::GetLocalAddress(IPEndPoint* address) const {
  return socket_.GetLocalAddress(address);
}

void QuicUDPServerSocket::AllowAddressReuse() {
  allow_address_reuse_ = true;
}

void QuicUDPServerSocket::AllowPortReuse() {
  allow_port_reuse_ = true;
}

void QuicUDPServerSocket::AllowBroadcast() {
  allow_broadcast_ = true;
}

int QuicUDPServerSocket::JoinGroup(const IPAddress& group_address) const {
  return socket_.JoinGroup(group_address);
}

int QuicUDPServerSocket::LeaveGroup(const IPAddress& group_address) const {
  return socket_.LeaveGroup(group_address);
}

int QuicUDPServerSocket::SetMulticastInterface(uint32_t interface_index) {
  return socket_.SetMulticastInterface(interface_index);
}

int QuicUDPServerSocket::SetMulticastTimeToLive(int time_to_live) {
  return socket_.SetMulticastTimeToLive(time_to_live);
}

int QuicUDPServerSocket::SetMulticastLoopbackMode(bool loopback) {
  return socket_.SetMulticastLoopbackMode(loopback);
}

int QuicUDPServerSocket::SetDiffServCodePoint(DiffServCodePoint dscp) {
  return socket_.SetDiffServCodePoint(dscp);
}

void QuicUDPServerSocket::DetachFromThread() {
  socket_.DetachFromThread();
}

void QuicUDPServerSocket::UseNonBlockingIO() {
#if defined(OS_WIN)
  socket_.UseNonBlockingIO();
#endif
}

const NetLogWithSource& QuicUDPServerSocket::NetLog() const {
  return net_log_;
}


}  // namespace net
