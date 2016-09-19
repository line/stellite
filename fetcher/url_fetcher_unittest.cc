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

#include <memory>

#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "stellite/test/http_test_server.h"
#include "net/proxy/proxy_config_service_fixed.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_fetcher_impl.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"
#include "net/url_request/url_request_context_getter.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

namespace {

class NoCacheURLRequestContextGetter : public URLRequestContextGetter {
 public:
  NoCacheURLRequestContextGetter(
      scoped_refptr<base::SingleThreadTaskRunner> network_task_runner)
      : network_task_runner_(network_task_runner) {
  }

  scoped_refptr<base::SingleThreadTaskRunner>
      GetNetworkTaskRunner() const override {
    return network_task_runner_;
  }

  URLRequestContext* GetURLRequestContext() override {
    if (!url_request_context_.get()) {
      std::unique_ptr<ProxyConfigService> proxy_config_service(
          new ProxyConfigServiceFixed(ProxyConfig()));

      URLRequestContextBuilder builder;
      builder.set_proxy_config_service(std::move(proxy_config_service));
      builder.DisableHttpCache();

      std::unique_ptr<URLRequestContext> request_context(builder.Build());
      url_request_context_.reset(request_context.release());
    }
    return url_request_context_.get();
  }

 protected:
  ~NoCacheURLRequestContextGetter() override {
    CHECK(network_task_runner_->BelongsToCurrentThread());
  }

 private:
  std::unique_ptr<HttpTestServer> http_server_;
  std::unique_ptr<URLRequestContext> url_request_context_;
  scoped_refptr<base::SingleThreadTaskRunner> network_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(NoCacheURLRequestContextGetter);
};

class WaitingURLFetcherDelegate : public URLFetcherDelegate {
 public:
  WaitingURLFetcherDelegate()
      : complete_(false),
        success_(false) {
  }

  void CreateFetcher(
      const GURL& url,
      const URLFetcher::RequestType request_type,
      scoped_refptr<net::URLRequestContextGetter> context_getter) {

    fetcher_ = URLFetcher::Create(url, request_type, this);
    fetcher_->SetRequestContext(context_getter.get());
  }

  URLFetcher* fetcher() {
    return fetcher_.get();
  }

  void WaitForComplete() {
    fetcher_->Start();
    run_loop_.Run();
  }

  void OnURLFetchComplete(const URLFetcher* source) override {
    run_loop_.Quit();

    EXPECT_FALSE(complete_);
    complete_ = true;

    EXPECT_FALSE(success_);
    success_ = source->GetResponseCode() == 200;
  }

  bool fetch_complete() {
    return complete_;
  }

  bool fetch_success() {
    return success_;
  }

 private:
  bool complete_;
  bool success_;
  std::unique_ptr<URLFetcher> fetcher_;
  base::RunLoop run_loop_;

  DISALLOW_COPY_AND_ASSIGN(WaitingURLFetcherDelegate);
};

} // namespace anonymous

class URLFetcherTest : public testing::Test {
 public:
  void SetUp() override {
    HttpTestServer::Params http_server_params;
    http_server_params.port = 9999;
    http_server_.reset(new HttpTestServer(http_server_params));
    http_server_->Start();

    delegate_.reset(new WaitingURLFetcherDelegate());
  }

  void TearDown() override {
    http_server_->Shutdown();
  }

  scoped_refptr<NoCacheURLRequestContextGetter>
      CreateSameThreadContextGetter() {
    CHECK(base::ThreadTaskRunnerHandle::Get().get());
    return scoped_refptr<NoCacheURLRequestContextGetter>(
        new NoCacheURLRequestContextGetter(
            base::ThreadTaskRunnerHandle::Get()));
  }

  std::unique_ptr<HttpTestServer> http_server_;
  std::unique_ptr<WaitingURLFetcherDelegate> delegate_;
};

TEST_F(URLFetcherTest, HttpGet) {
  delegate_->CreateFetcher(GURL("http://example.com:9999"),
                           URLFetcher::GET,
                           CreateSameThreadContextGetter());
  delegate_->WaitForComplete();
  EXPECT_TRUE(delegate_->fetch_complete());
  EXPECT_TRUE(delegate_->fetch_success());
}

} // namespace net
