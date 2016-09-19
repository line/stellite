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

#ifndef QUIC_CLIENT_NETWORK_TRANSACTION_CONSUMER_H_
#define QUIC_CLIENT_NETWORK_TRANSACTION_CONSUMER_H_

#include <memory>

#include "base/memory/ref_counted.h"
#include "base/timer/timer.h"
#include "net/base/request_priority.h"
#include "net/http/http_request_info.h"
#include "stellite/include/stellite_export.h"

namespace net {

class GrowableIOBuffer;
class HttpResponseInfo;
class HttpTransaction;
class IOBuffer;
class NetworkTransactionClient;
class NetworkTransactionConsumer;
class NetworkTransactionFactory;

// The delegate for transaction notice consumer
class STELLITE_EXPORT TransactionConsumerVisitor {
 public:
  virtual ~TransactionConsumerVisitor() {}

  virtual void OnTransactionTerminate(
      scoped_refptr<NetworkTransactionConsumer> transaction, int result)=0;

  virtual void OnTransactionStream(
      scoped_refptr<NetworkTransactionConsumer> transaction,
      const char* data, size_t len, int result)=0;
};

// net::NetworkTransactionConsumer has one life cycle between request and
// response. HTTP request and response has complex steps but Chromium's
// net::HttpTransaction implements all the steps. net::NetworkTransactionConsumer
// is just a callee of (Start, Read, DoneReading, GetResponseInfo of
// net::HttpTransaction
class STELLITE_EXPORT NetworkTransactionConsumer
    : public base::RefCounted<NetworkTransactionConsumer> {
 public:
  NetworkTransactionConsumer(const HttpRequestInfo& request,
                             const std::stringstream& request_body,
                             RequestPriority priority,
                             NetworkTransactionFactory* factory,
                             bool stream_response,
                             int timeout);

  void Start();

  void TearDown();

  const HttpResponseInfo* response_info() const;

  bool is_done() const {
    return state_ == DONE;
  }

  int error() const {
    return error_;
  }

  GrowableIOBuffer* buffer() {
    return contents_.get();
  }

  void set_visitor(TransactionConsumerVisitor* visitor) {
    visitor_ = visitor;
  }

 private:
  virtual ~NetworkTransactionConsumer();
  friend class base::RefCounted<NetworkTransactionConsumer>;

  enum State {
    IDLE,
    STARTING,
    READING,
    DONE,
  };

  // Each of these methods corresponds to State value.
  void DidStart(int result);
  void DidRead(int result);
  void DidFinish(int result);
  void Read();

  // HTTP request timeout callback. Each request has a timeout parameter.
  // This callback is called after the set timeout.
  void OnTransactionTimeout();

  void SetTransactionTimeout(int timeout_milliseconds);

  // HttpTransaction's IO complete callback. This is called only in two cases.
  // 1. When a transaction begins
  // 2. When a transaction is completed
  // Any other cases are errors.
  void OnIOComplete(int result);

  // Request information
  HttpRequestInfo request_;

  // Reqest payload
  std::string request_body_;
  std::unique_ptr<UploadDataStream> upload_data_stream_;
  RequestPriority priority_;

  NetworkTransactionFactory* transaction_factory_;

  State state_;
  std::unique_ptr<HttpTransaction> http_transaction_;
  scoped_refptr<IOBuffer> read_buf_;
  scoped_refptr<GrowableIOBuffer> contents_;
  int error_;

  TransactionConsumerVisitor* visitor_;

  // Whether a streamed response or not
  bool stream_response_;

  // Request timeout
  base::TimeTicks transaction_waiting_since_;
  int timeout_;
  std::unique_ptr<base::OneShotTimer> transaction_timer_;

  DISALLOW_COPY_AND_ASSIGN(NetworkTransactionConsumer);
};

} // namespace net

#endif
