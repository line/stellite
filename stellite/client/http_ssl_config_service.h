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

#ifndef STELLITE_CLIENT_HTTP_SSL_CONFIG_SERVICE_H_
#define STELLITE_CLIENT_HTTP_SSL_CONFIG_SERVICE_H_

#include "stellite/include/stellite_export.h"
#include "net/ssl/ssl_config_service.h"

namespace net {

// Support HTTP client certificate
class STELLITE_EXPORT HttpSSLConfigService
    : public SSLConfigService {
 public:
  explicit HttpSSLConfigService();

  void GetSSLConfig(SSLConfig* config) override;

 private:
  ~HttpSSLConfigService() override;

  void Init();

  SSLConfig ssl_config_;

  DISALLOW_COPY_AND_ASSIGN(HttpSSLConfigService);
};

} // namespace net

#endif // STELLITE_CLIENT_HTTP_SSL_CONFIG_SERVICE_H_
