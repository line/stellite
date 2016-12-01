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

#include "stellite/logging/logging_service.h"

#include "base/bind.h"
#include "base/files/file_util.h"
#include "stellite/logging/file_switch_per_date.h"
#include "stellite/logging/log_file_factory_default.h"
#include "stellite/logging/log_file_writer.h"

namespace stellite {

const char kAccessSeverity[] = "access";
const char kDebugSeverity[] = "debug";
const char kErrorSeverity[] = "error";
const char kLoggingThread[] = "logging_thread";
const char kUnknownSeverity[] = "unknown";
const char kLogDirectory[] = "log";

LoggingService::LoggingService() {
}

LoggingService::~LoggingService() {
  Shutdown();
}

// Static
LoggingService* LoggingService::GetInstance() {
  return base::Singleton<LoggingService>::get();
}

bool LoggingService::Initialize(const base::FilePath& logging_root) {
  if (logging_thread_.get()) {
    LOG(ERROR) << "Logging service has already been initialized";
    return false;
  }

  // Check whether a logging directory exists or not
  base::FilePath logging_dir = base::MakeAbsoluteFilePath(logging_root);
  logging_dir = logging_dir.Append(kLogDirectory);

  if (!base::DirectoryExists(logging_dir)) {
    if (!base::CreateDirectory(logging_dir)) {
      LOG(ERROR) << "Failed to create a logging directory";
      return false;
    }
  }

  // Initialize log file factory
  log_file_factory_.reset(new LogFileFactoryDefault(logging_dir));

  // Initialize log file write
  for (LogSeverity severity = 0; severity < LOGGING_END; ++severity) {
    std::string service_tag(GetTag(severity));
    FileSwitch* file_switch = new FileSwitchPerDate(service_tag);
    std::unique_ptr<LogFileWriter> writer(
        new LogFileWriter(log_file_factory_.get(), file_switch));
    log_writer_list_.push_back(std::move(writer));
  }

  // Start IO thread
  logging_thread_.reset(new base::Thread(kLoggingThread));
  base::Thread::Options options(base::MessageLoop::TYPE_IO, 0);
  logging_thread_->StartWithOptions(options);

  return true;
}

void LoggingService::Shutdown() {
  if (!logging_thread_.get() || logging_thread_->IsRunning()) {
    return;
  }

  logging_thread_->Stop();
  logging_thread_.reset();
}

void LoggingService::Write(LogSeverity log_severity,
                           const std::string& message) {
  if (!logging_thread_.get()) {
    return;
  }

  logging_thread_->task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&LoggingService::OnWrite,
                 base::Unretained(this),
                 log_severity,
                 message));
}

std::string LoggingService::GetTag(LogSeverity log_severity) {
  switch (log_severity) {
    case LOGGING_ACCESS:
      return kAccessSeverity;
    case LOGGING_ERROR:
      return kErrorSeverity;
    case LOGGING_DEBUG:
      return kDebugSeverity;
    default:
      NOTREACHED();
      return kUnknownSeverity;
  }
  NOTREACHED();
  return kUnknownSeverity;
}

void LoggingService::OnWrite(LogSeverity severity, const std::string& message) {
  if (log_writer_list_.size() <= severity) {
    return;
  }

  LogFileWriter* writer = log_writer_list_[severity].get();
  writer->PostStream(message);
}

} // namespace stellite
