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

#include "node_binder/socket/udp_server.h"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "net/base/net_errors.h"
#include "net/base/sockaddr_storage.h"

namespace stellite {

UDPServer::UDPServer()
    : socket_(new UDPSocket()) {
}

UDPServer::~UDPServer() {}

int UDPServer::Open(net::AddressFamily address_family) {
  return socket_->Open(address_family);
}

void UDPServer::Close() {
  socket_->Close();
}

int UDPServer::Listen(const net::IPEndPoint& address) {
  return socket_->Listen(address);
}

int UDPServer::AllowAddressReuse() {
  return socket_->AllowAddressReuse();
}

int UDPServer::AllowPortReuse() {
  return socket_->AllowPortReuse();
}

int UDPServer::SetReceiveBufferSize(int32_t size) {
  return socket_->SetReceiveBufferSize(size);
}

int UDPServer::SetSendBufferSize(int32_t size) {
  return socket_->SetSendBufferSize(size);
}

int UDPServer::GetLocalAddress(net::IPEndPoint* address) const {
  return socket_->GetLocalAddress(address);
}

int UDPServer::RecvFrom(net::IOBuffer* buf, int buf_len,
                        net::IPEndPoint* address,
                        const net::CompletionCallback& callback) {
  return socket_->RecvFrom(buf, buf_len, address, callback);
}

int UDPServer::SendTo(net::IOBuffer* buf, int buf_len,
                      const net::IPEndPoint* address,
                      const net::CompletionCallback& callback) {
  return socket_->SendTo(buf, buf_len, address, callback);
}

}  // namespace stellite
