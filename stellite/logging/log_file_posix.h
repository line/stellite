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

#ifndef STELLITE_LOGGING_LOG_FILE_POSIX_H_
#define STELLITE_LOGGING_LOG_FILE_POSIX_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/message_loop/message_loop.h"
#include "stellite/logging/log_file.h"

namespace stellite {

namespace test {
class LogFilePosixPeer;
}

class LogFilePosix : public LogFile {
 public:
  LogFilePosix(const base::FilePath& path);

  ~LogFilePosix() override;

  // Implementation for LogFile
  int Open() override;

  void Close() override;

  int Write(const char* buf,
            size_t buf_len,
            const net::CompletionCallback& callback) override;

 private:
  friend class test::LogFilePosixPeer;

  class WriteWatcher : public base::MessageLoopForIO::Watcher {
   public:
    explicit WriteWatcher(LogFilePosix* file_log);
    ~WriteWatcher() override;

    void OnFileCanReadWithoutBlocking(int /* fd */) override;

    void OnFileCanWriteWithoutBlocking(int /* fd */) override;

   private:
    LogFilePosix* log_file_;

    base::MessageLoopForIO::FileDescriptorWatcher write_file_watcher_;

    DISALLOW_COPY_AND_ASSIGN(WriteWatcher);
  };

  void DoWriteCallback(int rv);
  void DidCompleteWrite();

  int InternalOpen(const base::FilePath& path);
  int InternalWrite(const char* buf, size_t buf_len);

  const base::FilePath log_path_;

  // Logging file descriptor
  int file_descriptor_;

  // Write watcher
  WriteWatcher write_watcher_;

  // Write file descriptor watcher
  base::MessageLoopForIO::FileDescriptorWatcher write_file_watcher_;

  // Buffer used by InteranlWrite() to retry a Write request
  size_t write_buf_len_;
  char* write_buf_;

  // External callback; called when write is complete.
  net::CompletionCallback write_callback_;

  DISALLOW_COPY_AND_ASSIGN(LogFilePosix);
};

} // namespace stellite

#endif
