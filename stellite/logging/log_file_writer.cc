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

#include "stellite/logging/log_file_writer.h"

#include "base/location.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "stellite/logging/file_switch_per_date.h"
#include "stellite/logging/log_file.h"
#include "stellite/logging/log_file_factory.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"

namespace stellite {

LogFileWriter::LogFileWriter(LogFileFactory* log_file_factory,
                             FileSwitch* file_switch)
    : file_switch_(file_switch),
      write_pending_(false),
      synchronous_write_count_(0),
      log_file_factory_(log_file_factory),
      weak_factory_(this) {
}

LogFileWriter::~LogFileWriter() {
}

void LogFileWriter::PostStream(const std::string& log) {
  if (log.size() == 0) {
    LOG(ERROR) << "The log size is invalid";
    return;
  }

  // Push a backlog message into a queue to serialize a log message
  log_queue_.push(log);

  StartWriting();
}

void LogFileWriter::StartWriting() {
  if (log_queue_.empty()) {
    return;
  }

  if (write_pending_) {
    return;
  }

  write_pending_ = true;

  // Replace the log file if the file name was expired
  if (!file_switch_->CheckName(filename_)) {
    filename_ = file_switch_->CurrentName();

    if (log_file_.get()) {
      log_file_->Close();
      log_file_.reset();
    }
  }

  if (!log_file_.get()) {
    log_file_.reset(log_file_factory_->CreateLogFile(filename_));
    int result = log_file_->Open();
    if (result != net::OK) {
      LOG(ERROR) << "Failed to open the logging file: " << result;

      write_pending_ = false;

      log_file_->Close();
      log_file_.reset();
      return;
    }
  }

  const std::string& log = log_queue_.front();
  int result = log_file_->Write(log.c_str(),
                                log.size(),
                                base::Bind(&LogFileWriter::OnWriteComplete,
                                           base::Unretained(this)));

  if (result == net::ERR_IO_PENDING) {
    synchronous_write_count_ = 0;
    return;
  }

  if (5 < ++synchronous_write_count_) {
    synchronous_write_count_ = 0;
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::Bind(&LogFileWriter::OnWriteComplete,
                   weak_factory_.GetWeakPtr(),
                   result));
  } else {
    OnWriteComplete(result);
  }
}

void LogFileWriter::OnWriteComplete(int result) {
  write_pending_ = false;

  log_queue_.pop();

  if (result < 0) {
    LOG(ERROR) << "Log writer write failed: " << net::ErrorToString(result);
    return;
  }

  if (log_queue_.empty()) {
    return;
  }

  StartWriting();
}

} // namespace stellite
