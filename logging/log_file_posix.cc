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

#include "stellite/logging/log_file_posix.h"

#include <fcntl.h>
#include <unistd.h>

#include "base/files/file_util.h"
#include "base/posix/eintr_wrapper.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"

namespace stellite {

const int kInvalidFd = -1;

LogFilePosix::LogFilePosix(const base::FilePath& log_path)
    : log_path_(log_path),
      file_descriptor_(kInvalidFd),
      write_watcher_(this),
      write_buf_len_(0) {
}

LogFilePosix::~LogFilePosix() {
  Close();
}

int LogFilePosix::Open() {
  DCHECK_EQ(file_descriptor_, kInvalidFd);
  int fd = InternalOpen(log_path_);
  if (fd == kInvalidFd) {
    return net::MapSystemError(errno);
  }

  if (!base::SetNonBlocking(fd)) {
    int result = net::MapSystemError(errno);
    LOG(INFO) << "Failed to set non-blocking in the log file: " << result;
    Close();
    return result;
  }

  file_descriptor_ = fd;
  return net::OK;
}

void LogFilePosix::Close() {
  if (file_descriptor_ == kInvalidFd) {
    return;
  }

  write_buf_ = nullptr;
  write_buf_len_ = 0;
  write_callback_.Reset();

  bool ok = write_file_watcher_.StopWatchingFileDescriptor();
  DCHECK(ok);

  IGNORE_EINTR(close(file_descriptor_));
  file_descriptor_ = kInvalidFd;
}

int LogFilePosix::Write(const char* buf,
                        size_t buf_len,
                        const net::CompletionCallback& callback) {
  int nwrite = InternalWrite(buf, buf_len);
  if (nwrite == static_cast<int>(buf_len)) {
    return nwrite;
  }

  if (!base::MessageLoopForIO::current()->WatchFileDescriptor(
          file_descriptor_,
          true /* persistent */,
          base::MessageLoopForIO::WATCH_WRITE,
          &write_file_watcher_,
          &write_watcher_)) {
    LOG(ERROR) << "WatchFileDescriptor failed on write";
    int result = net::MapSystemError(errno);
    return result;
  }

  int skip_offset = 0;
  if (nwrite > 0) {
    skip_offset += nwrite;
  }

  write_buf_ = const_cast<char*>(buf + skip_offset);
  write_buf_len_ = buf_len - skip_offset;
  write_callback_ = callback;
  return net::ERR_IO_PENDING;
}

LogFilePosix::WriteWatcher::WriteWatcher(LogFilePosix* log_file)
  : log_file_(log_file) {
}

LogFilePosix::WriteWatcher::~WriteWatcher() {
}

void LogFilePosix::WriteWatcher::OnFileCanReadWithoutBlocking(int /* fd */) {
}

void LogFilePosix::WriteWatcher::OnFileCanWriteWithoutBlocking(int /* fd */) {
  if (log_file_->write_callback_.is_null()) {
    return;
  }
  log_file_->DidCompleteWrite();
}

void LogFilePosix::DoWriteCallback(int result) {
  DCHECK_NE(result, net::ERR_IO_PENDING);
  DCHECK(!write_callback_.is_null());

  net::CompletionCallback callback = write_callback_;
  write_callback_.Reset();
  callback.Run(result);
}

void LogFilePosix::DidCompleteWrite() {
  int result = InternalWrite(write_buf_, write_buf_len_);

  if (result != net::ERR_IO_PENDING) {
    write_buf_ = nullptr;
    write_buf_len_ = 0;
    write_file_watcher_.StopWatchingFileDescriptor();
    DoWriteCallback(result);
  }
}

int LogFilePosix::InternalOpen(const base::FilePath& path) {
  int flag = O_CREAT | O_WRONLY | O_APPEND;
  int mode = S_IRUSR | S_IWUSR;
  int result = HANDLE_EINTR(open(path.value().c_str(), flag, mode));
  if (result < 0) {
    return kInvalidFd;
  }

  return result;
}

int LogFilePosix::InternalWrite(const char* buf, size_t buf_len) {
  int result = HANDLE_EINTR(::write(file_descriptor_, buf, buf_len));
  if (result < 0) {
    result = net::MapSystemError(errno);
  }
  return result;
}

} // namespace stellite
