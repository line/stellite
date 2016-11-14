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

#ifndef STELLITE_CLIENT_NETWORK_TRANSACTION_FACTORY_H_
#define STELLITE_CLIENT_NETWORK_TRANSACTION_FACTORY_H_

#include <memory>

#include "base/power_monitor/power_observer.h"
#include "stellite/include/stellite_export.h"
#include "net/base/net_export.h"
#include "net/base/request_priority.h"
#include "net/http/http_network_session.h"

namespace net {
class HttpTransaction;

// The factory classs for net::HttpTransaction
class STELLITE_EXPORT NetworkTransactionFactory
    : public base::PowerObserver {
 public:
  NetworkTransactionFactory(HttpNetworkSession* session);
  ~NetworkTransactionFactory() override;

  int CreateTransaction(RequestPriority priority,
                        std::unique_ptr<HttpTransaction>* trans);
  HttpNetworkSession* GetSession();

  // Implements LINE game's necessary
  void OnCancel();

 private:
  HttpNetworkSession* session_;

  DISALLOW_COPY_AND_ASSIGN(NetworkTransactionFactory);
};

} // namespace net

#endif // STELLITE_CLIENT_NETWORK_CLIENT_TRANSACTION_FACTORY_H_
