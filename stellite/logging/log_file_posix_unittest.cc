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

#include <fcntl.h>

#include <memory>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/run_loop.h"
#include "stellite/logging/log_file_posix.h"
#include "net/base/net_errors.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace stellite {

const int kInvalidFd = -1;

namespace test {

class LogFilePosixPeer {
 public:
  static int OpenPipe(LogFilePosix* log_file) {
    if (log_file->file_descriptor_ != kInvalidFd) {
      close(log_file->file_descriptor_);
      log_file->file_descriptor_ = kInvalidFd;
    }

    int pipe_fds[2] = {kInvalidFd, };
    int ret = ::pipe(pipe_fds);
    if (ret == -1) {
      LOG(ERROR) << "Failed to open pipe: " << ret;
      return kInvalidFd;
    }

    if (!base::SetNonBlocking(pipe_fds[1])) {
      LOG(ERROR) << "Failed to set non-blocking on pipe[0]"
          << net::MapSystemError(errno);
      return kInvalidFd;
    }

    log_file->file_descriptor_ = pipe_fds[1];
    return pipe_fds[0];
  }

  // Static void ClosePipe(LogFilePosix*
};

} // namespace test

class LogFilePosixTest : public testing::Test {
 public:
  void SetUp() override {
    pipe_out_ = kInvalidFd;
  }

  void TearDown() override {
    if (pipe_out_ != kInvalidFd) {
      close(pipe_out_);
    }
    pipe_out_ = kInvalidFd;

    log_file_->Close();
  }

  bool Initialize() {
    log_file_.reset(new LogFilePosix(base::FilePath()));
    pipe_out_ = test::LogFilePosixPeer::OpenPipe(log_file_.get());
    return pipe_out_ != kInvalidFd;
  }

  void OnWrite(int result) {
    run_loop_.Quit();
  }

  base::RunLoop run_loop_;
  std::unique_ptr<LogFilePosix> log_file_;
  int pipe_out_;
};

TEST_F(LogFilePosixTest, BasicTest) {
  EXPECT_TRUE(Initialize());

  std::string logging_message("example");
  int ret = log_file_->Write(logging_message.data(),
                             logging_message.size(),
                             base::Bind(&LogFilePosixTest::OnWrite,
                                        base::Unretained(this)));
  if (ret == net::ERR_IO_PENDING) {
    run_loop_.Run();
  }

  std::vector<char> read_buffer(64, 0);
  ret = ::read(pipe_out_, &read_buffer[0], read_buffer.size());
  EXPECT_TRUE(ret > 0);

  std::string read_message(&read_buffer[0], ret);
  EXPECT_EQ(read_message, "example");
}

} // namespace stellite
