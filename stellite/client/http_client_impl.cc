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

#include "stellite/client/http_client_impl.h"

#include "stellite/client/network_transaction_client.h"
#include "stellite/include/http_client.h"
#include "stellite/include/http_request.h"
#include "stellite/include/http_response.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_info.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_response_info.h"
#include "url/gurl.h"

const int kDefaultRequestTimeout = 60 * 1000; // 30 seconds

namespace stellite {

HttpClientImpl::HttpClientImpl(
    net::NetworkTransactionClient* network_transaction_client,
    HttpResponseDelegate* response_delegate)
    : network_transaction_client_(network_transaction_client),
      response_delegate_(response_delegate) {
  DCHECK(network_transaction_client_.get());
}

HttpClientImpl::~HttpClientImpl() {
  TearDown();
}

int HttpClientImpl::Request(const HttpRequest& request) {
  return Request(request, kDefaultRequestTimeout);
}

int HttpClientImpl::Request(const HttpRequest& request, int timeout) {
  return network_transaction_client_->Request(request,
                                              response_delegate_,
                                              net::DEFAULT_PRIORITY,
                                              /* stream_resposne */ false,
                                              timeout);
}

int HttpClientImpl::Stream(const HttpRequest& request) {
  return Stream(request, kDefaultRequestTimeout);
}

int HttpClientImpl::Stream(const HttpRequest& request, int timeout) {
  return network_transaction_client_->Request(request,
                                              response_delegate_,
                                              net::DEFAULT_PRIORITY,
                                              /* stream_resposne */ true,
                                              timeout);
}

void HttpClientImpl::TearDown() {
  // Reset visitor
  DCHECK(network_transaction_client_.get());
  network_transaction_client_->TearDown();
}

} // namespace stellite
