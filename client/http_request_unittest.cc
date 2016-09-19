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

#include "stellite/include/http_request.h"

#include <memory>
#include <string>

#include "testing/gtest/include/gtest/gtest.h"

namespace stellite {

const char kKnownHeader[] = "known_header";
const char kUnknownHeader[] = "unknown_header";
const char kKnownValue[] = "known_value";

class HttpRequestTest : public testing::Test {
 public:
  HttpRequestTest() {}

  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(HttpRequestTest, RequestHeaderValidation) {
  std::unique_ptr<HttpRequestHeader> header(new HttpRequestHeader());
  EXPECT_FALSE(header->HasHeader(kUnknownHeader));

  std::string value;
  EXPECT_FALSE(header->GetHeader(kUnknownHeader, &value));

  header->SetHeader(kKnownHeader, kKnownValue);
  EXPECT_TRUE(header->HasHeader(kKnownHeader));
  EXPECT_TRUE(header->GetHeader(kKnownHeader, &value));
  EXPECT_EQ(value, kKnownValue);

  header->RemoveHeader(kKnownHeader);
  EXPECT_FALSE(header->HasHeader(kKnownHeader));

  header->SetHeader(kKnownHeader, kKnownValue);
  header->ClearHeader();
  EXPECT_FALSE(header->HasHeader(kKnownHeader));
}

TEST_F(HttpRequestTest, RequestHeaderToString) {
  std::unique_ptr<HttpRequestHeader> header(new HttpRequestHeader());

  header->SetHeader(kKnownHeader, kKnownValue);
  EXPECT_EQ(header->ToString(), "known_header: known_value\r\n\r\n");
}

} // namespace stellite
