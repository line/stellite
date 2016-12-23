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

#ifndef STELLITE_SERVER_QUIC_PROXY_WORKER_H_
#define STELLITE_SERVER_QUIC_PROXY_WORKER_H_

#include <memory>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "net/base/ip_endpoint.h"
#include "net/quic/core/crypto/quic_crypto_server_config.h"
#include "net/quic/core/quic_clock.h"
#include "net/quic/core/quic_config.h"
#include "net/quic/core/quic_protocol.h"
#include "stellite/server/server_config.h"

namespace base {
class SingleThreadTaskRunner;
class Thread;
}

namespace net {
class EphemeralKeySource;
class IOBufferWithSize;
class QuicChromiumAlarmFactory;
class QuicChromiumConnectionHelper;
class QuicProxyDispatcher;
class QuicServerConfig;
class QuicServerConfigProtobuf;
class QuicUDPServerSocket;

namespace test {
class QuicProxyWorkerPeer;
}

// net::QuicProxyWorker has two thread and UDP server socket.
// One is QUIC dispatching thread, last one is backend fetching thread.
// The other role of net::QuicProxyWorker is to read datagram and hand-over
// datagram to quic_dispatcher.
class NET_EXPORT QuicProxyWorker {
 public:
  QuicProxyWorker(
      scoped_refptr<base::SingleThreadTaskRunner> dispatch_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> http_fetch_task_runner,
      const IPEndPoint& bind_address,
      const QuicConfig& quic_config,
      const ServerConfig& server_config,
      const QuicVersionVector& supported_versions,
      std::unique_ptr<ProofSource> proof_source);

  virtual ~QuicProxyWorker();

  bool Initialize();
  bool Initialize(const std::vector<QuicServerConfigProtobuf*>& protobufs);
  void SetStrikeRegisterNoStartupPeriod();
  void SetEphemeralKeySource(EphemeralKeySource* key_source);

  void Start();
  void Stop();

 protected:
  const QuicConfig& quic_config() {
    return quic_config_;
  }

  const QuicCryptoServerConfig* crypto_config() {
    return &crypto_config_;
  }

  const ServerConfig& server_config() {
    return server_config_;
  }

  scoped_refptr<base::SingleThreadTaskRunner> dispatch_task_runner() {
    return dispatch_task_runner_;
  }

  scoped_refptr<base::SingleThreadTaskRunner> http_fetch_task_runner() {
    return http_fetch_task_runner_;
  }

 private:
  friend class test::QuicProxyWorkerPeer;

  void StartOnBackground();
  void StartReading();
  void StopReading();

  void OnReadComplete(int result);

  const int dispatch_continuity_;

  // Worker thread
  scoped_refptr<base::SingleThreadTaskRunner> dispatch_task_runner_;

  // Fetcher thread
  scoped_refptr<base::SingleThreadTaskRunner> http_fetch_task_runner_;

  // Used by the helper_ to time alarms.
  QuicClock clock_;

  // Accepts data from the framer and demuxes clients to sessions.
  std::unique_ptr<QuicProxyDispatcher> dispatcher_;

  // Used to manage the message loop. Owned by dispatcher
  QuicChromiumConnectionHelper* helper_;

  // Used to manage the message loop. Owned by dispatcher
  QuicChromiumAlarmFactory* alarm_factory_;

  // Listening socket. Also used for outbound client communication.
  std::unique_ptr<QuicUDPServerSocket> socket_;

  // Listening address.
  const IPEndPoint bind_address_;

  // config_ contains non-crypto parameters that are negotiated in the crypto
  // handshake.
  const QuicConfig& quic_config_;

  // crypto_config_ contains crypto parameters for the handshake.
  QuicCryptoServerConfig crypto_config_;

  // Server config
  const ServerConfig& server_config_; /* not owned */

  QuicVersionManager version_manager_;

  // The address that the server listens on.
  IPEndPoint server_address_;

  // Keeps track of whether a read is currently in-flight, after which
  // OnReadComplete will be called.
  bool read_pending_;

  // The number of iterations of the read loop that have been completed
  // synchronously and without posting a new task to the message loop.
  int synchronous_read_count_;

  // The target buffer of the current read.
  scoped_refptr<IOBufferWithSize> read_buffer_;

  // The source address of the current read.
  IPEndPoint client_address_;

  base::WeakPtrFactory<QuicProxyWorker> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(QuicProxyWorker);
};

} // namespace net

#endif // STELLITE_SERVER_QUIC_PROXY_WORKER_H_
