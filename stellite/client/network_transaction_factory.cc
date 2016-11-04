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

#include "stellite/client/network_transaction_factory.h"

#include "net/http/http_network_transaction.h"
#include "net/quic/core/crypto/quic_server_info.h"

namespace net {

NetworkTransactionFactory::NetworkTransactionFactory(
    HttpNetworkSession* session)
    :  session_(session) {
}

NetworkTransactionFactory::~NetworkTransactionFactory() {
  //session_->CloseAllConnections();
}

int NetworkTransactionFactory::CreateTransaction(
    RequestPriority priority,
    std::unique_ptr<HttpTransaction>* transaction) {

  transaction->reset(new HttpNetworkTransaction(priority, session_));

  return OK;
}

HttpNetworkSession* NetworkTransactionFactory::GetSession() {
  return session_;
}

void NetworkTransactionFactory::OnCancel() {
  session_->CloseAllConnections();
}

} // namespace net
