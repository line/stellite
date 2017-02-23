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

#ifndef NODE_BINDER_SOCKET_UDP_SOCKET_H_
#define NODE_BINDER_SOCKET_UDP_SOCKET_H_

#include "base/threading/non_thread_safe.h"
#include "net/base/address_family.h"
#include "net/base/completion_callback.h"
#include "net/base/io_buffer.h"
#include "net/log/net_log_with_source.h"
#include "net/base/ip_endpoint.h"
#include "net/socket/socket_descriptor.h"
#include "node/uv.h"

namespace stellite {

class UDPSocket : public base::NonThreadSafe {
 public:
  UDPSocket();
  ~UDPSocket();

  int Open(net::AddressFamily address_family);
  void Close();

  int AllowAddressReuse();
  int AllowPortReuse();

  int GetLocalAddress(net::IPEndPoint* address) const;
  int GetPeerAddress(net::IPEndPoint* address) const;
  int SetReceiveBufferSize(int32_t size);
  int SetSendBufferSize(int32_t size);

  // listen specified address
  int Connect(const net::IPEndPoint& address);
  int Listen(const net::IPEndPoint& address);
  int RecvFrom(net::IOBuffer* buf, int buf_len, net::IPEndPoint* address,
               const net::CompletionCallback& callback);
  int SendTo(net::IOBuffer* buf, int buf_len, const net::IPEndPoint* address,
             const net::CompletionCallback& callback);

  bool is_opened() { return is_opened_; }

  const net::NetLogWithSource& NetLog() const { return net_log_; }

 private:
  static void OnCanIO(uv_poll_t* handle, int status, int events);

  void OnCanRead();
  void OnCanWrite();
  void ResetWatchIOEvent(int consuemd);

  void DidCompleteRead();
  void DidCompleteWrite();
  void DoReadCallback(int rv);
  void DoWriteCallback(int rv);

  int InternalConnect(const net::IPEndPoint& address);
  int InternalSendTo(net::IOBuffer* buf, int buf_len,
                     const net::IPEndPoint* address);
  int InternalRecvFrom(net::IOBuffer* buf, int buf_len,
                       net::IPEndPoint* address);

  bool WatchIOEvent(int event);

  net::SocketDescriptor socket_;
  int address_family_;

  bool is_opened_;

  mutable std::unique_ptr<net::IPEndPoint> remote_address_;
  mutable std::unique_ptr<net::IPEndPoint> local_address_;

  // uv handle
  std::unique_ptr<uv_poll_t> io_event_handle_;
  int uv_event_mask_;

  scoped_refptr<net::IOBuffer> read_buf_;
  int read_buf_len_;
  net::IPEndPoint* recv_from_address_;

  scoped_refptr<net::IOBuffer> write_buf_;
  int write_buf_len_;
  std::unique_ptr<net::IPEndPoint> send_to_address_;

  // external read complete callback
  net::CompletionCallback read_callback_;

  // external write complete callback
  net::CompletionCallback write_callback_;

  net::NetLogWithSource net_log_;

  DISALLOW_COPY_AND_ASSIGN(UDPSocket);
};

}  // namespace stellite

#endif  // NODE_BINDER_SOCKET_UDP_SOCKET_H_

