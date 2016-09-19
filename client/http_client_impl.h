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

#ifndef STELLITE_CLIENT_HTTP_CLIENT_IMPL_H_
#define STELLITE_CLIENT_HTTP_CLIENT_IMPL_H_

#include "stellite/include/http_client.h"

#include "base/memory/ref_counted.h"
#include "stellite/client/network_transaction_client.h"
#include "stellite/include/stellite_export.h"

namespace net {
class NetworkTransactionClient;
}

namespace stellite {
class HttpResponseDelegate;

class STELLITE_EXPORT HttpClientImpl : public HttpClient {
 public:
  HttpClientImpl(net::NetworkTransactionClient* transaction_client,
                 HttpResponseDelegate* response_delegate);
  ~HttpClientImpl() override;

  int Request(const HttpRequest& request) override;

  int Request(const HttpRequest& request, int timeout) override;

  int Stream(const HttpRequest& request) override;

  int Stream(const HttpRequest& request, int timeout) override;

  void TearDown();

 private:
  scoped_refptr<net::NetworkTransactionClient> network_transaction_client_;

  HttpResponseDelegate* response_delegate_;

  DISALLOW_COPY_AND_ASSIGN(HttpClientImpl);
};

} // namespace stellite

#endif // STELLITE_CLIENT_HTTP_CLIENT_IMPL_H_
