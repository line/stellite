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

#ifndef QUIC_CLIENT_NETWORK_TRANSACTION_CLIENT_H_
#define QUIC_CLIENT_NETWORK_TRANSACTION_CLIENT_H_

#include <list>
#include <map>
#include <memory>
#include <queue>

#include "base/memory/ref_counted.h"
#include "stellite/client/network_transaction_consumer.h"
#include "stellite/include/stellite_export.h"
#include "net/filter/filter.h"
#include "net/http/http_request_info.h"
#include "net/http/transport_security_state.h"
#include "net/log/net_log.h"

namespace stellite {
class HttpResponseDelegate;
struct HttpRequest;
struct HttpResponse;
} // namespace stellite

namespace base {
class SingleThreadTaskRunner;
} // namespace base

namespace net {
class NetworkTransactionConsumer;
class NetworkTransactionFactory;

// NetworkTransactionClient manages the life cycle of an HTTP request.
class STELLITE_EXPORT NetworkTransactionClient
    : public TransactionConsumerVisitor,
      public base::RefCounted<NetworkTransactionClient> {
 public:
  // An active request's default capacity is 10.
  NetworkTransactionClient(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      NetworkTransactionFactory* factory,
      size_t active_request_capacity = 10);

  int Request(const stellite::HttpRequest& request,
              stellite::HttpResponseDelegate* delegate,
              const RequestPriority priority,
              bool stream_response,
              int request_timeout);

  void TearDown();

 private:
  friend class base::RefCounted<NetworkTransactionClient>;
  ~NetworkTransactionClient() override;

  typedef std::queue<scoped_refptr<NetworkTransactionConsumer>> PendingEntries;
  typedef std::list<scoped_refptr<NetworkTransactionConsumer>> ActiveEntries;
  typedef std::map<NetworkTransactionConsumer*,
      std::pair<int, stellite::HttpResponseDelegate*>> ResponseDelegateMap;

  // Implement a TransactionConsumerVisitor method.
  void OnTransactionTerminate(
      scoped_refptr<NetworkTransactionConsumer> transaction,
      int result) override;

  void OnTransactionStream(
      scoped_refptr<NetworkTransactionConsumer> transaction,
      const char* stream_data, size_t len, int result) override;

  void OnTransactionBegin(
      int request_id,
      scoped_refptr<NetworkTransactionConsumer> transaction,
      stellite::HttpResponseDelegate* delegate);

  void OnPendingTransactionBegin();

  // Tear down transaction consumer.
  void DeleteTransaction(scoped_refptr<NetworkTransactionConsumer> transaction);

  // Convert stellite::HttpRequest to net::HttpResponseInfo
  bool RewriteHttpResponse(stellite::HttpResponse* dest,
                           const HttpResponseInfo* src);

  // Convert stellite::HttpRequest to net::HttpRequestInfo
  bool RewriteHttpRequest(HttpRequestInfo* desc,
                          const stellite::HttpRequest* src);

  // Decode the payload if |Content-Encoding| response header is gzip or
  // deflate or something else ...
  bool Decode(Filter::FilterType filter_type,
              const char* encoded, const int size,
              std::unique_ptr<std::vector<char>>* out);

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  // HTTP client request will reach its maximum request count if there are more
  // HTTP client requests than NetworkTransactionClient can store requests in
  // pending state. If concurrent requests count drops below this capacity,
  // those pending requests will move to active requests and will be processed.
  ActiveEntries active_entries_;
  PendingEntries pending_entries_;
  ResponseDelegateMap response_delegate_map_;

  NetworkTransactionFactory* transaction_factory_;

  size_t active_request_capacity_;
  int last_request_id_;

  DISALLOW_COPY_AND_ASSIGN(NetworkTransactionClient);
};

} // namespace net

#endif
