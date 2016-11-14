//
// Copyright 2016 LINE Corporation
//

#include "stellite/include/logging.h"
#include "testing/gtest/include/gtest/gtest.h"

class LoggingTest: public testing::Test {
 public:
  LoggingTest() {}
  ~LoggingTest() override {}

  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(LoggingTest, BasicLogging) {
  LLOG(INFO) << "hello world";
}