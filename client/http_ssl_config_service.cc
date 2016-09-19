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

#include "stellite/client/http_ssl_config_service.h"

namespace net {

HttpSSLConfigService::HttpSSLConfigService() {
  Init();
}

HttpSSLConfigService::~HttpSSLConfigService() {
}

void HttpSSLConfigService::GetSSLConfig(SSLConfig* config) {
  *config = ssl_config_;
}

void HttpSSLConfigService::Init() {
  // Chromium will send an empty SSL client certificate
  ssl_config_.send_client_cert = true;
}

} // namespace net
