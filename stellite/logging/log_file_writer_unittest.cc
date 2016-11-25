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
#include <sstream>

#include "base/message_loop/message_loop.h"
#include "stellite/logging/file_switch.h"
#include "stellite/logging/log_file.h"
#include "stellite/logging/log_file_factory.h"
#include "stellite/logging/log_file_writer.h"
#include "net/base/net_errors.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace stellite {

class MockLogFile : public LogFile {
 public:
  class LogFileVisitor {
   public:
    virtual ~LogFileVisitor() {}

    virtual void OnWrite(const char* buf, size_t buf_len) = 0;
  };

  MockLogFile(LogFileVisitor* visitor) : visitor_(visitor) {}
  ~MockLogFile() override {}

  int Open() override {
    return net::OK;
  }

  void Close() override {}

  int Write(const char* buf, size_t buf_len,
            const net::CompletionCallback& callback) override {
    if (visitor_) {
      visitor_->OnWrite(buf, buf_len);
    }
    return net::OK;
  }

 private:
  LogFileVisitor* visitor_;

  DISALLOW_COPY_AND_ASSIGN(MockLogFile);
};

class MockLogFileFactory
    : public LogFileFactory, public MockLogFile::LogFileVisitor {
 public:
  MockLogFileFactory() {}
  ~MockLogFileFactory() override {}

  LogFile* CreateLogFile(const std::string& filename) override {
    return new MockLogFile(this);
  }

  void OnWrite(const char* buf, size_t buf_len) override {
    if (buf_len > 0) {
      stream_.write(buf, buf_len);
    }
  }

  const std::stringstream& writed_stream() {
    return stream_;
  }

 private:
  std::stringstream stream_;

  DISALLOW_COPY_AND_ASSIGN(MockLogFileFactory);
};

class LogFileWriterTest : public testing::Test {
 public:
  void SetUp() override {
    file_factory_.reset(new MockLogFileFactory());
    log_writer_.reset(new LogFileWriter(file_factory_.get(), new FileSwitch()));
  }

  void TearDown() override {
  }

  std::unique_ptr<MockLogFileFactory> file_factory_;
  std::unique_ptr<LogFileWriter> log_writer_;
};

TEST_F(LogFileWriterTest, BasicWrite) {
  log_writer_->PostStream("example");
  base::MessageLoop::current()->RunUntilIdle();
  std::string writed = file_factory_->writed_stream().str();
  EXPECT_EQ(writed, "example");
}

} // namespace stellite
