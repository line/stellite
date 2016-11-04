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

#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread_task_runner_handle.h"
#include "stellite/fetcher/http_fetcher.h"
#include "stellite/fetcher/http_request_context_getter.h"
#include "stellite/test/http_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

namespace {

std::unique_ptr<HttpFetcher> CreateFetcher() {
  return std::unique_ptr<HttpFetcher>(
      new HttpFetcher(
          new HttpRequestContextGetter(
              base::ThreadTaskRunnerHandle::Get())));
}

class WaitingHttpFetcherVisitor : public HttpFetcherDelegate {
 public:
  WaitingHttpFetcherVisitor()
      : complete_(false),
        success_(false),
        weak_factory_(this) {
  }

  ~WaitingHttpFetcherVisitor() override {}

  void OnFetchComplete(const URLFetcher* source, int64_t msec) override {
    run_loop_.Quit();

    EXPECT_FALSE(complete_);
    complete_ = true;

    EXPECT_FALSE(success_);
    success_ = source->GetResponseCode() == 200;
  }

  void OnFetchTimeout(int64_t msec) override {
    EXPECT_FALSE(complete_);
    complete_ = true;
  }

  void FetchAndWait(HttpFetcher* fetcher, const GURL& url,
                    const URLFetcher::RequestType request_type) {
    HttpRequestHeaders headers;
    fetcher->Fetch(url, request_type, headers, std::string(), 1000,
                   weak_factory_.GetWeakPtr());
    Wait();
  }

  void Wait() {
    run_loop_.Run();
  }

  bool fetch_complete() {
    return complete_;
  }

  bool fetch_success() {
    return success_;
  }

  base::WeakPtr<HttpFetcherDelegate> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

 private:
  bool complete_;
  bool success_;

  std::unique_ptr<HttpFetcher> http_fetcher_;
  base::RunLoop run_loop_;

  base::WeakPtrFactory<WaitingHttpFetcherVisitor> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WaitingHttpFetcherVisitor);
};

} // namespace anonymous

class HttpFetcherTest : public testing::Test {
 public:
  void SetUp() override {
    HttpTestServer::Params params;
    params.port = 9999;
    http_server_.reset(new HttpTestServer(params));
    http_server_->Start();

    visitor_.reset(new WaitingHttpFetcherVisitor());
  }

  void TearDown() override {
    http_server_->Shutdown();
  }

  std::unique_ptr<HttpTestServer> http_server_;
  std::unique_ptr<WaitingHttpFetcherVisitor> visitor_;
};

TEST_F(HttpFetcherTest, FetchGet) {
  std::unique_ptr<HttpFetcher> fetcher(CreateFetcher());
  visitor_->FetchAndWait(fetcher.get(),
                         GURL("http://example.com:9999"), URLFetcher::GET);

  EXPECT_TRUE(visitor_->fetch_complete());
  EXPECT_TRUE(visitor_->fetch_success());
}

TEST_F(HttpFetcherTest, FetchWithDelegateInvalidate) {
  std::unique_ptr<HttpFetcher> fetcher(CreateFetcher());

  {
    WaitingHttpFetcherVisitor visitor;
    HttpRequestHeaders headers;
    fetcher->Fetch(GURL("http://example.com:9999"), URLFetcher::GET, headers,
                   std::string(), 500, visitor.GetWeakPtr());
  }
  base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(501));
}

} // namespace net
