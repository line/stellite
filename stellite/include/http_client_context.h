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

#ifndef QUIC_INCLUDE_HTTP_CLIENT_CONTEXT_H_
#define QUIC_INCLUDE_HTTP_CLIENT_CONTEXT_H_

#include <string>
#include <vector>

#if defined(ANDROID)
#include <jni.h>
#endif

#include "stellite_export.h"

namespace stellite {
class HttpClient;
class HttpResponseDelegate;

// HttpClientContext is HttpClient's factory that shares HTTP client's
// request service like networking thread, certificate verifier,
// host name resolver, proxy service, and SSL config. It contains everything
// related to an HTTP request.
class STELLITE_EXPORT HttpClientContext {
 public:
  struct STELLITE_EXPORT Params {
    Params();
    Params(const Params& other);
    ~Params();

    bool using_quic;
    bool using_http2;
    bool ignore_certificate_errors;
    bool enable_http2_alternative_service_with_different_host;
    bool enable_quic_alternative_service_with_different_host;

    std::string proxy_host;
    std::vector<std::string> origins_to_force_quic_on;

    bool using_disk_cache;
    bool using_memory_cache;
    int max_cache_size;
    std::string disk_cache_path;
  };

  explicit HttpClientContext(const Params& params);
  virtual ~HttpClientContext();

  bool Initialize();
  bool TearDown();

  void CancelAll();

  // the factory function but HttpClient object ownership was on
  // HttpClientContext. so do release client with ReleaseHttpClient function
  // not using delete
  HttpClient* CreateHttpClient(HttpResponseDelegate* visitor);
  void ReleaseHttpClient(HttpClient* client);

  static bool ResetCertBundle(const std::string& pem_path);

#if defined(ANDROID)
  // In Android JNI applications, InitVM function must call
  // jint JNI_OnLoad(JavaVM*vm, void* reserved) initialized function
  static bool InitVM(JavaVM* vm);
#endif

 private:
  class ContextImpl;
  ContextImpl* context_impl_;

  // DISALLOW_COPY_AND_ASSIGN
  HttpClientContext(const HttpClientContext&);
  void operator=(const HttpClientContext&);
};

} // namespace stellite

#endif // QUIC_INCLUDE_HTTP_CLIENT_CONTEXT_H_
