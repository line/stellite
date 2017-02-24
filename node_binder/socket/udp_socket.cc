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

#include "node_binder/socket/udp_socket.h"

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

namespace {

void OnReleaseHandle(uv_handle_t* handle) {
  delete handle;
}

}  // namespace anonymous

UDPSocket::UDPSocket()
    : socket_(net::kInvalidSocket),
      address_family_(0),
      is_opened_(false),
      uv_event_mask_(0),
      read_buf_len_(0),
      recv_from_address_(nullptr),
      write_buf_len_(0) {
}

UDPSocket::~UDPSocket() {}

int UDPSocket::Open(net::AddressFamily address_family) {
  DCHECK(CalledOnValidThread());
  DCHECK_EQ(socket_, net::kInvalidSocket);
  DCHECK(!is_opened_);
  address_family_ = ConvertAddressFamily(address_family);

  socket_ = net::CreatePlatformSocket(address_family_, SOCK_DGRAM, 0);
  if (socket_ == net::kInvalidSocket) {
    return net::MapSystemError(errno);
  }

  if (!base::SetNonBlocking(socket_)) {
    const int err = net::MapSystemError(errno);
    Close();
    return err;
  }

  io_event_handle_.reset(new uv_poll_t);
  io_event_handle_->data = this;
  if (uv_poll_init(uv_default_loop(), io_event_handle_.get(), socket_) != 0) {
    return net::MapSystemError(errno);
  }

  return net::OK;
}

void UDPSocket::Close() {
  DCHECK(CalledOnValidThread());

  if (socket_ == net::kInvalidSocket) {
    return;
  }

  address_family_ = 0;
  is_opened_ = false;

  read_buf_ = nullptr;
  read_buf_len_ = 0;
  recv_from_address_ = nullptr;
  write_buf_ = nullptr;
  write_buf_len_ = 0;
  send_to_address_.reset();

  read_callback_.Reset();
  write_callback_.Reset();

  uv_poll_stop(io_event_handle_.get());

  uv_close(reinterpret_cast<uv_handle_t*>(io_event_handle_.release()),
           &OnReleaseHandle);

  close(socket_);
  socket_ = net::kInvalidSocket;
}

int UDPSocket::Connect(const net::IPEndPoint& address) {
  DCHECK_NE(socket_, net::kInvalidSocket);
  int rv = InternalConnect(address);
  is_opened_= (rv == net::OK);
  return rv;
}

int UDPSocket::Listen(const net::IPEndPoint& address) {
  DCHECK_NE(socket_, net::kInvalidSocket);
  DCHECK(!is_opened_);
  DCHECK(CalledOnValidThread());

  net::SockaddrStorage storage;
  if (!address.ToSockAddr(storage.addr, &storage.addr_len)) {
    LOG(ERROR) << "invalid bind address.";
    return net::ERR_ADDRESS_INVALID;
  }

  int rv = bind(socket_, storage.addr, storage.addr_len);
  if (rv == 0) {
    is_opened_ = true;
    return net::OK;
  }

  int last_error = errno;
#if defined(OS_MACOSX)
  if (last_error == EADDRNOTAVAIL) {
    return net::ERR_ADDRESS_IN_USE;
  }
#endif
  return net::MapSystemError(last_error);
}

int UDPSocket::AllowAddressReuse() {
  DCHECK_NE(socket_, net::kInvalidSocket);
  DCHECK(CalledOnValidThread());
  DCHECK(!is_opened_);
  int allow_addr_reuse = 1;
  int rv = setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &allow_addr_reuse,
                      sizeof(allow_addr_reuse));
  return rv == 0 ? net::OK : net::MapSystemError(errno);
}

int UDPSocket::AllowPortReuse() {
  DCHECK_NE(socket_, net::kInvalidSocket);
  DCHECK(CalledOnValidThread());
  DCHECK(!is_opened_);

  int allow_reuse_port = 1;
  int rv = setsockopt(socket_, SOL_SOCKET, SO_REUSEPORT, &allow_reuse_port,
                      sizeof(allow_reuse_port));
  return rv == 0 ? net::OK : net::MapSystemError(errno);
}

int UDPSocket::SetReceiveBufferSize(int32_t size) {
  DCHECK_NE(socket_, net::kInvalidSocket);
  DCHECK(CalledOnValidThread());

  int rv = setsockopt(socket_, SOL_SOCKET, SO_RCVBUF,
                      reinterpret_cast<const char*>(&size), sizeof(size));
  return rv == 0 ? net::OK : net::MapSystemError(errno);
}

int UDPSocket::SetSendBufferSize(int32_t size) {
  DCHECK_NE(socket_, net::kInvalidSocket);
  DCHECK(CalledOnValidThread());

  int rv = setsockopt(socket_, SOL_SOCKET, SO_SNDBUF,
                      reinterpret_cast<const char*>(&size), sizeof(size));
  return rv == 0 ? net::OK : net::MapSystemError(errno);
}

int UDPSocket::GetLocalAddress(net::IPEndPoint* address) const {
  DCHECK_NE(socket_, net::kInvalidSocket);
  DCHECK(CalledOnValidThread());

  if (!is_opened_) {
    return net::ERR_SOCKET_NOT_CONNECTED;
  }

  if (!local_address_.get()) {
    net::SockaddrStorage storage;
    if (getsockname(socket_, storage.addr, &storage.addr_len)) {
      return net::MapSystemError(errno);
    }

    std::unique_ptr<net::IPEndPoint> address(new net::IPEndPoint());
    if (!address->FromSockAddr(storage.addr, storage.addr_len)) {
      return net::ERR_ADDRESS_INVALID;
    }

    local_address_ = std::move(address);
  }

  *address = *local_address_;

  return net::OK;
}

int UDPSocket::GetPeerAddress(net::IPEndPoint* address) const {
  DCHECK(CalledOnValidThread());
  DCHECK(address);
  if (!is_opened_) {
    return net::ERR_SOCKET_NOT_CONNECTED;
  }

  if (!remote_address_.get()) {
    net::SockaddrStorage storage;
    if (getpeername(socket_, storage.addr, &storage.addr_len)) {
      return net::MapSystemError(errno);
    }
    std::unique_ptr<net::IPEndPoint> address(new net::IPEndPoint());
    if (!address->FromSockAddr(storage.addr, storage.addr_len)) {
      return net::ERR_ADDRESS_INVALID;
    }
    remote_address_ = std::move(address);
  }

  *address = *remote_address_;
  return net::OK;
}

int UDPSocket::RecvFrom(net::IOBuffer* buf, int buf_len,
                        net::IPEndPoint* address,
                        const net::CompletionCallback& callback) {
  DCHECK_NE(socket_, net::kInvalidSocket);
  DCHECK(CalledOnValidThread());

  CHECK(read_callback_.is_null());

  DCHECK(!callback.is_null());
  DCHECK_GT(buf_len, 0);

  int nread = InternalRecvFrom(buf, buf_len, address);
  if (nread != net::ERR_IO_PENDING) {
    return nread;
  }

  // process IO_PENDING status
  if (!WatchIOEvent(UV_READABLE)) {
    LOG(ERROR) << "watch socket io failed on read";
    int rv = net::MapSystemError(errno);
    return rv;
  }

  read_buf_ = buf;
  read_buf_len_ = buf_len;
  recv_from_address_ = address;
  read_callback_ = callback;
  return net::ERR_IO_PENDING;
}

int UDPSocket::SendTo(net::IOBuffer* buf, int buf_len,
                      const net::IPEndPoint* address,
                      const net::CompletionCallback& callback) {
  DCHECK(CalledOnValidThread());
  DCHECK_NE(socket_, net::kInvalidSocket);

  CHECK(write_callback_.is_null());
  DCHECK(!callback.is_null());
  DCHECK_GT(buf_len, 0);

  int nread = InternalSendTo(buf, buf_len, address);
  if (nread != net::ERR_IO_PENDING) {
    return nread;
  }

  if (!WatchIOEvent(UV_WRITABLE)) {
    LOG(ERROR) << "watch socket io failed on write";
    int rv = net::MapSystemError(errno);
    return rv;
  }

  write_buf_ = buf;
  write_buf_len_ = buf_len;
  DCHECK(!send_to_address_.get());
  if (address) {
    send_to_address_.reset(new net::IPEndPoint(*address));
  }
  write_callback_ = callback;
  return net::ERR_IO_PENDING;
}

void UDPSocket::OnCanRead() {
  DCHECK(CalledOnValidThread());
  if (read_callback_.is_null()) {
    return;
  }
  DidCompleteRead();
}

void UDPSocket::OnCanWrite() {
  DCHECK(CalledOnValidThread());
  if (write_callback_.is_null()) {
    return;
  }
  DidCompleteWrite();
}

void UDPSocket::DidCompleteRead() {
  DCHECK(CalledOnValidThread());
  int rv = InternalRecvFrom(read_buf_.get(), read_buf_len_,
                            recv_from_address_);
  if (rv == net::ERR_IO_PENDING) {
    WatchIOEvent(UV_READABLE);
  } else {
    read_buf_ = nullptr;
    read_buf_len_ = 0;
    recv_from_address_ = nullptr;
    DoReadCallback(rv);
  }
}

void UDPSocket::DidCompleteWrite() {
  DCHECK(CalledOnValidThread());

  int rv = InternalSendTo(write_buf_.get(), write_buf_len_,
                          send_to_address_.get());
  if (rv == net::ERR_IO_PENDING) {
    WatchIOEvent(UV_WRITABLE);
  } else {
    write_buf_ = nullptr;
    write_buf_len_ = 0;
    send_to_address_.reset();
    DoWriteCallback(rv);
  }
}

void UDPSocket::DoReadCallback(int rv) {
  DCHECK(CalledOnValidThread());

  DCHECK_NE(rv, net::ERR_IO_PENDING);
  DCHECK(!read_callback_.is_null());

  net::CompletionCallback callback = read_callback_;
  read_callback_.Reset();
  callback.Run(rv);
}

void UDPSocket::DoWriteCallback(int rv) {
  DCHECK(CalledOnValidThread());

  DCHECK_NE(rv, net::ERR_IO_PENDING);
  DCHECK(!write_callback_.is_null());

  net::CompletionCallback callback = write_callback_;
  write_callback_.Reset();
  callback.Run(rv);
}

int UDPSocket::InternalConnect(const net::IPEndPoint& address) {
  DCHECK(CalledOnValidThread());
  DCHECK(!is_opened_);
  DCHECK(!remote_address_.get());

  net::SockaddrStorage storage;
  if (!address.ToSockAddr(storage.addr, &storage.addr_len)) {
    return net::ERR_ADDRESS_INVALID;
  }

  int rv = HANDLE_EINTR(connect(socket_, storage.addr, storage.addr_len));
  if (rv < 0) {
    return net::MapSystemError(errno);
  }
  remote_address_.reset(new net::IPEndPoint(address));
  return rv;
}

int UDPSocket::InternalSendTo(net::IOBuffer* buf, int buf_len,
                              const net::IPEndPoint* address) {
  DCHECK(CalledOnValidThread());

  net::SockaddrStorage storage;
  struct sockaddr* addr = storage.addr;
  if (!address) {
    addr = NULL;
    storage.addr_len = 0;
  } else {
    if (!address->ToSockAddr(storage.addr, &storage.addr_len)) {
      int result = net::ERR_ADDRESS_INVALID;
      return result;
    }
  }

  int rv = HANDLE_EINTR(sendto(socket_,
                               buf->data(),
                               buf_len,
                               0, addr,
                               storage.addr_len));
  if (rv < 0) {
    rv = net::MapSystemError(errno);
  }
  return rv;
}

int UDPSocket::InternalRecvFrom(net::IOBuffer* buf, int buf_len,
                                net::IPEndPoint* address) {
  DCHECK(CalledOnValidThread());

  int flags = 0;
  net::SockaddrStorage storage;
  int bytes_transferred = HANDLE_EINTR(recvfrom(socket_,
                                                buf->data(),
                                                buf_len,
                                                flags,
                                                storage.addr,
                                                &storage.addr_len));
  int rv = 0;
  if (bytes_transferred >= 0) {
    rv = bytes_transferred;
    if (address && !address->FromSockAddr(storage.addr, storage.addr_len)) {
      rv = net::ERR_ADDRESS_INVALID;
    }
  } else {
    rv = net::MapSystemError(errno);
  }

  return rv;
}

void UDPSocket::OnCanIO(uv_poll_t* handle, int status, int events) {
  UDPSocket* server = static_cast<UDPSocket*>(handle->data);
  DCHECK(server);

  // reset event
  server->ResetWatchIOEvent(events);

  if (events & UV_READABLE) {
    server->OnCanRead();
  }

  if (events & UV_WRITABLE) {
    server->OnCanWrite();
  }
}

bool UDPSocket::WatchIOEvent(int io_events) {
  DCHECK(CalledOnValidThread());
  uv_event_mask_ |= io_events;
  if (uv_poll_start(io_event_handle_.get(), uv_event_mask_, OnCanIO) != 0) {
    return false;
  }

  return true;
}

void UDPSocket::ResetWatchIOEvent(int consumed) {
  DCHECK(CalledOnValidThread());
  uv_event_mask_ = uv_event_mask_ & (~consumed);
  CHECK(!uv_poll_start(io_event_handle_.get(), uv_event_mask_, OnCanIO));
}

}  // namespace stellite
