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

#ifndef NODE_BINDER_SOCKET_UDP_SERVER_H_
#define NODE_BINDER_SOCKET_UDP_SERVER_H_

#include <memory>

#include "base/threading/non_thread_safe.h"
#include "net/base/completion_callback.h"
#include "net/base/io_buffer.h"
#include "net/base/ip_endpoint.h"
#include "node_binder/socket/udp_socket.h"

namespace stellite {

class UDPServer : public base::NonThreadSafe {
 public:
  UDPServer();
  ~UDPServer();

  int Open(net::AddressFamily address_family);
  void Close();

  int AllowAddressReuse();
  int AllowPortReuse();

  int SetReceiveBufferSize(int32_t size);
  int SetSendBufferSize(int32_t size);

  int GetLocalAddress(net::IPEndPoint* address) const;

  // listen specified address
  int Listen(const net::IPEndPoint& address);

  int RecvFrom(net::IOBuffer* buf, int buf_len, net::IPEndPoint* address,
               const net::CompletionCallback& callback);

  int SendTo(net::IOBuffer* buf, int buf_len, const net::IPEndPoint* address,
             const net::CompletionCallback& callback);
 private:
  std::unique_ptr<UDPSocket> socket_;

  DISALLOW_COPY_AND_ASSIGN(UDPServer);
};

}  // namespace stellite

#endif  // NODE_BINDER_SOCKET_NODE_UDP_SERVER_H_
