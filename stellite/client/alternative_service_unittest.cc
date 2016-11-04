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

#include "base/logging.h"
#include "net/spdy/spdy_alt_svc_wire_format.h"
#include "testing/gtest/include/gtest/gtest.h"

class AlternativeServiceTest : public testing::Test {
 public:
  AlternativeServiceTest() {}
  ~AlternativeServiceTest() override {}

  void SetUp() override {
  }

  void TearDown() override {
  }
};

TEST_F(AlternativeServiceTest, ParseSimpleAltSvc) {
  net::SpdyAltSvcWireFormat::AlternativeServiceVector service_vector;
  EXPECT_TRUE(net::SpdyAltSvcWireFormat::ParseHeaderFieldValue(
          "quic=\"127.0.0.1:4430\"",
          &service_vector));
  EXPECT_EQ(service_vector.size(), static_cast<size_t>(1));

  net::SpdyAltSvcWireFormat::AlternativeService&
      alternative_service = service_vector[0];
  EXPECT_EQ(alternative_service.protocol_id, "quic");
  EXPECT_EQ(alternative_service.host, "127.0.0.1");
  EXPECT_EQ(alternative_service.port, 4430);
}

TEST_F(AlternativeServiceTest, ParseFullAltSvcHeader) {
  net::SpdyAltSvcWireFormat::AlternativeServiceVector alternative_services;
  EXPECT_TRUE(net::SpdyAltSvcWireFormat::ParseHeaderFieldValue(
          "quic=\":443\"; p=\"1\"; ma=604800",
          &alternative_services));
}
