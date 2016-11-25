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

#include "testing/gtest/include/gtest/gtest.h"

namespace stellite {

class HttpClientContextTest : public testing::Test {};


TEST_F(HttpClientContextTest, InitContext) {
  HttpClientContext::Params params;

  std::unique_ptr<HttpClientContext> context(new HttpClientContext(params));

  EXPECT_TRUE(context->Initialize());
  EXPECT_TRUE(context->TearDown());
}

TEST_F(HttpClientContextTest, DuplicatedInitializeTearDown) {
  HttpClientContext::Params params;

  std::unique_ptr<HttpClientContext> context(new HttpClientContext(params));
  EXPECT_TRUE(context->Initialize());
  EXPECT_FALSE(context->Initialize());
  EXPECT_TRUE(context->TearDown());
}

TEST_F(HttpClientContextTest, ReuseHttpClientContext) {
  HttpClientContext::Params params;

  std::unique_ptr<HttpClientContext> context(new HttpClientContext(params));

  EXPECT_TRUE(context->Initialize());
  EXPECT_TRUE(context->TearDown());

  // Retry
  EXPECT_TRUE(context->Initialize());
  EXPECT_TRUE(context->TearDown());
}

TEST_F(HttpClientContextTest, Cancel) {
  HttpClientContext::Params params;

  std::unique_ptr<HttpClientContext> context(new HttpClientContext(params));
  EXPECT_TRUE(context->Initialize());

  context->Cancel();

  EXPECT_TRUE(context->TearDown());
}

// TODO(@snibug): Test Pause(), Resume(), CreateHttpClient(),
// ReleaseHttpClient()

} // namespace stellite
