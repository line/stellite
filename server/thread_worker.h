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

#ifndef STELLITE_THREAD_QUIC_DISPATCH_WORKER_H_
#define STELLITE_THREAD_QUIC_DISPATCH_WORKER_H_

#include <memory>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "net/base/ip_endpoint.h"
#include "net/log/net_log.h"
#include "net/quic/crypto/quic_crypto_server_config.h"
#include "net/quic/quic_clock.h"
#include "net/quic/quic_config.h"
#include "net/quic/quic_protocol.h"
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
class QuicServerConfig;
class QuicServerConfigProtobuf;
class QuicUDPServerSocket;
class ThreadDispatcher;

namespace test {
class ThreadWorkerPeer;
}

// net::ThreadWorker has two thread and UDP server socket.
// One is QUIC dispatching thread, last one is backend fetching thread.
// The other role of net::ThreadWorker is to read datagram and hand-over
// datagram to quic_dispatcher.
class NET_EXPORT ThreadWorker {
 public:
  ThreadWorker(
      const IPEndPoint& bind_address,
      const QuicConfig& quic_config,
      const ServerConfig& server_config,
      const QuicVersionVector& supported_versions,
      ProofSource* proof_source);

  virtual ~ThreadWorker();

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

  const QuicVersionVector& supported_versions() {
    return supported_versions_;
  }

  scoped_refptr<base::SingleThreadTaskRunner> dispatch_task_runner();

  scoped_refptr<base::SingleThreadTaskRunner> fetch_task_runner();

 private:
  friend class test::ThreadWorkerPeer;

  void StartOnBackground();

  void StartReading();

  void StopReading();

  void OnReadComplete(int result);

  const int dispatch_continuity_;

  // Worker thread
  std::unique_ptr<base::Thread> dispatch_thread_;

  // Fetcher thread
  std::unique_ptr<base::Thread> fetch_thread_;

  // Used by the helper_ to time alarms.
  QuicClock clock_;

  // Accepts data from the framer and demuxes clients to sessions.
  std::unique_ptr<ThreadDispatcher> dispatcher_;

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

  // This vector contains QUIC versions which we currently support.
  // This should be ordered such that the highest supported version is the first
  // element, with subsequent elements in descending order (versions can be
  // skipped as necessary).
  QuicVersionVector supported_versions_;

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

  // The log to use for the socket.
  NetLog net_log_;

  base::WeakPtrFactory<ThreadWorker> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ThreadWorker);
};

} // namespace net

#endif // STELLITE_THREAD_QUIC_DISPATCH_WORKER_H_
