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

#include "stellite/include/http_client_context.h"

#include <memory>
#include <map>

#include "base/at_exit.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread.h"
#include "stellite/client/http_client_impl.h"
#include "stellite/fetcher/http_fetcher.h"
#include "stellite/fetcher/http_request_context_getter.h"

#if defined(ANDROID)
#include "base/android/base_jni_registrar.h"
#include "base/android/context_utils.h"
#include "base/android/jni_android.h"
#include "base/android/jni_utils.h"
#include "net/android/net_jni_registrar.h"
#endif

// Default cache size
const char* kNetworkThreadName = "network thread";

namespace stellite {

class HttpClientContext::ContextImpl {
 public:
  ContextImpl(const Params& context_params);
  ~ContextImpl();

  bool Init();
  bool Teardown();
  void CancelAll();

  HttpClient* CreateHttpClient(HttpResponseDelegate* response_delegate);
  void ReleaseHttpClient(HttpClient* client);

 private:
  using HttpClientMap = std::map<HttpClient*, std::unique_ptr<HttpClient>>;

  void StartOnIOThread();
  void TeardownOnIOThread();

  const Params context_params_;

  static base::AtExitManager s_at_exit_manager_;

  // all HTTP request are working on network thread
  std::unique_ptr<base::Thread> network_thread_;

  scoped_refptr<HttpRequestContextGetter> http_request_context_getter_;
  std::unique_ptr<HttpFetcher> http_fetcher_;

  HttpClientMap http_client_map_;

  DISALLOW_COPY_AND_ASSIGN(ContextImpl);
};

base::AtExitManager HttpClientContext::ContextImpl::s_at_exit_manager_;

HttpClientContext::ContextImpl::ContextImpl(const Params& context_params)
    : context_params_(context_params) {
}

HttpClientContext::ContextImpl::~ContextImpl() {
  Teardown();
}

bool HttpClientContext::ContextImpl::Init() {
  if (network_thread_.get()) {
    return false;
  }

  base::Thread::Options network_thread_options;
  network_thread_options.message_loop_type = base::MessageLoop::TYPE_IO;
  network_thread_.reset(new base::Thread(kNetworkThreadName));
  network_thread_->StartWithOptions(network_thread_options);

  HttpRequestContextGetter::Params params;
  params.proxy_host = context_params_.proxy_host;
  params.enable_http2 = context_params_.using_http2;
  params.enable_quic = context_params_.using_quic;
  params.ignore_certificate_errors = context_params_.ignore_certificate_errors;
  params.using_disk_cache = context_params_.using_disk_cache;
  params.cache_max_size = 0;
  params.sdch_enable = true;
  params.throttling_enable = true;

  http_request_context_getter_ = new HttpRequestContextGetter(
      params, network_thread_->task_runner());
  http_fetcher_.reset(new HttpFetcher(http_request_context_getter_));

  return true;
}

bool HttpClientContext::ContextImpl::Teardown() {
  if (!network_thread_.get() || !network_thread_->IsRunning()) {
    return false;
  }

  network_thread_->Stop();
  network_thread_.reset();

  return true;
}

void HttpClientContext::ContextImpl::CancelAll() {
  http_fetcher_->CancelAll();
}

HttpClient* HttpClientContext::ContextImpl::CreateHttpClient(
    HttpResponseDelegate* response_delegate) {
  HttpClient* client = new HttpClientImpl(http_fetcher_.get(),
                                          response_delegate);
  return client;
}

void HttpClientContext::ContextImpl::ReleaseHttpClient(HttpClient* client) {
  delete client;
}

HttpClientContext::Params::Params()
    : using_quic(true),
      using_http2(true),
      using_disk_cache(false),
      ignore_certificate_errors(false),
      enable_http2_alternative_service_with_different_host(true),
      enable_quic_alternative_service_with_different_host(true) {
}

HttpClientContext::Params::Params(const Params& other) = default;

HttpClientContext::Params::~Params() {}


HttpClientContext::HttpClientContext(const Params& context_params)
  : context_impl_(new HttpClientContext::ContextImpl(context_params)) {
}

HttpClientContext::~HttpClientContext() {
  TearDown();

  if (context_impl_) {
    delete context_impl_;
    context_impl_ = NULL;
  }
}

bool HttpClientContext::Initialize() {
  return context_impl_->Init();
}

bool HttpClientContext::TearDown() {
  // Release context implementation
  return context_impl_->Teardown();
}

void HttpClientContext::CancelAll() {
  context_impl_->CancelAll();
}

HttpClient* HttpClientContext::CreateHttpClient(HttpResponseDelegate* visitor) {
  return context_impl_->CreateHttpClient(visitor);
}

void HttpClientContext::ReleaseHttpClient(stellite::HttpClient* client) {
  CHECK(client);
  context_impl_->ReleaseHttpClient(client);
}

#if defined(ANDROID)

// Static
bool HttpClientContext::InitVM(JavaVM* vm) {
  if (base::android::IsVMInitialized()) {
    return false;
  }

  // Initialize
  base::android::InitVM(vm);

  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::InitReplacementClassLoader(
      env, base::android::GetClassLoader(env));

  if (!base::android::RegisterJni(env)) {
    return false;
  }

  if (!net::android::RegisterJni(env)) {
    return false;
  }

  return true;
}

#endif

bool HttpClientContext::ResetCertBundle(const std::string& filename) {
  return false;
}

} // namespace stellite
