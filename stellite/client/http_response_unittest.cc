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

#include "stellite/include/http_response.h"

#include <memory>
#include <string>

#include "base/memory/ref_counted.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace stellite {

class HttpResponseTest : public testing::Test {
 public:
  HttpResponseTest() {}

  void SetUp() override {}
  void TearDown() override {}
};

void HeadersToRaw(std::string* headers) {
  std::replace(headers->begin(), headers->end(), '\n', '\0');
  if (!headers->empty()) {
    *headers += '\0';
  }
}

TEST_F(HttpResponseTest, ResponseHeaderValidation) {
  std::string raw_header = "HTTP/1.1 200 OK\n"
                           "Content-Type: text/html; charset=utf-8\n";
  HeadersToRaw(&raw_header);

  std::unique_ptr<HttpResponseHeader> headers(
      new HttpResponseHeader(raw_header));
  EXPECT_TRUE(headers->HasHeader("Content-Type"));
}

TEST_F(HttpResponseTest, ResponseHeaderEnumerate) {
  std::string raw_header = "HTTP/1.1 200 OK\n"
                           "Content-Type: application/json\n";
  HeadersToRaw(&raw_header);
  std::unique_ptr<HttpResponseHeader> header(
      new HttpResponseHeader(raw_header));

  size_t iterator = 0;
  std::string value;
  EXPECT_TRUE(header->EnumerateHeader(&iterator, "content-type", &value));
  EXPECT_EQ(value, "application/json");
}

TEST_F(HttpResponseTest, ResponseHeaderIterate) {
  std::string raw_header = "HTTP/1.1 200 OK\n"
                           "Content-Type: application/json\n"
                           "Alt-Svc: quic='example.com:443'\n";

  HeadersToRaw(&raw_header);
  std::unique_ptr<HttpResponseHeader>
      header(new HttpResponseHeader(raw_header));

  size_t iterator = 0;
  std::string key, value;
  for (int i = 0; i < 2; ++i) {
    EXPECT_TRUE(header->EnumerateHeaderLines(&iterator, &key, &value));
  }
  EXPECT_FALSE(header->EnumerateHeaderLines(&iterator, &key, &value));
}

TEST_F(HttpResponseTest, ResponseCopyConstruct) {
  HttpResponse source;
  source.response_code = 200;
  source.content_length = 0;
  source.status_text = "OK";
  source.mime_type = "application/json";
  source.charset = "utf-8";
  source.was_cached = true;
  source.server_data_unavailable = false;
  source.network_accessed = true;
  source.was_fetched_via_spdy = false;
  source.was_npn_negotiated = false;
  source.was_fetched_via_proxy = false;

  HttpResponse dest(source);
  EXPECT_EQ(source.response_code, dest.response_code);
  EXPECT_EQ(source.content_length, dest.content_length);
  EXPECT_EQ(source.status_text, dest.status_text);
  EXPECT_EQ(source.mime_type, dest.mime_type);
  EXPECT_EQ(source.charset, dest.charset);
  EXPECT_EQ(source.was_cached, dest.was_cached);
  EXPECT_EQ(source.server_data_unavailable, dest.server_data_unavailable);
  EXPECT_EQ(source.network_accessed, dest.network_accessed);
  EXPECT_EQ(source.was_fetched_via_spdy, dest.was_fetched_via_spdy);
  EXPECT_EQ(source.was_npn_negotiated, dest.was_npn_negotiated);
  EXPECT_EQ(source.was_fetched_via_proxy, dest.was_fetched_via_proxy);

  HttpResponse clone;
  clone = source;
  EXPECT_EQ(source.response_code, clone.response_code);
  EXPECT_EQ(source.content_length, clone.content_length);
  EXPECT_EQ(source.status_text, clone.status_text);
  EXPECT_EQ(source.mime_type, clone.mime_type);
  EXPECT_EQ(source.charset, clone.charset);
  EXPECT_EQ(source.was_cached, clone.was_cached);
  EXPECT_EQ(source.server_data_unavailable, clone.server_data_unavailable);
  EXPECT_EQ(source.network_accessed, clone.network_accessed);
  EXPECT_EQ(source.was_fetched_via_spdy, clone.was_fetched_via_spdy);
  EXPECT_EQ(source.was_npn_negotiated, clone.was_npn_negotiated);
  EXPECT_EQ(source.was_fetched_via_proxy, clone.was_fetched_via_proxy);
}

} // namespace stellite
