// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stellite/server/server_per_connection_packet_writer.h"

#include "base/bind.h"
#include "stellite/server/server_packet_writer.h"

namespace net {

ServerPerConnectionPacketWriter::ServerPerConnectionPacketWriter(
    ServerPacketWriter* shared_writer)
    : shared_writer_(shared_writer),
      connection_(nullptr),
      weak_factory_(this) {}

ServerPerConnectionPacketWriter::~ServerPerConnectionPacketWriter() {}

QuicPacketWriter* ServerPerConnectionPacketWriter::shared_writer() const {
  return shared_writer_;
}

WriteResult ServerPerConnectionPacketWriter::WritePacket(
    const char* buffer,
    size_t buf_len,
    const IPAddress& self_address,
    const IPEndPoint& peer_address,
    PerPacketOptions* options) {

  return shared_writer_->WritePacketWithCallback(
      buffer, buf_len, self_address, peer_address, options,
      base::Bind(&ServerPerConnectionPacketWriter::OnWriteComplete,
                 weak_factory_.GetWeakPtr()));
}

bool ServerPerConnectionPacketWriter::IsWriteBlockedDataBuffered() const {
  return shared_writer_->IsWriteBlockedDataBuffered();
}

bool ServerPerConnectionPacketWriter::IsWriteBlocked() const {
  return shared_writer_->IsWriteBlocked();
}

void ServerPerConnectionPacketWriter::SetWritable() {
  shared_writer_->SetWritable();
}

void ServerPerConnectionPacketWriter::OnWriteComplete(WriteResult result) {
  if (connection_ && result.status == WRITE_STATUS_ERROR) {
    connection_->OnWriteError(result.error_code);
  }
}

QuicByteCount ServerPerConnectionPacketWriter::GetMaxPacketSize(
    const IPEndPoint& peer_address) const {
  return shared_writer_->GetMaxPacketSize(peer_address);
}

}  // namespace net