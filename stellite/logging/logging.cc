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

#include "stellite/logging/logging.h"

#include "base/process/process.h"
#include "base/strings/safe_sprintf.h"
#include "base/time/time.h"
#include "stellite/logging/logging_service.h"

namespace stellite {

FileLogMessage::FileLogMessage(LogSeverity severity, const char* file, int line)
    : severity_(severity),
      file_(file),
      line_(line) {
}

FileLogMessage::~FileLogMessage() {
  std::string filename(file_);
  size_t last_slash_pos = filename.find_last_of("\\/");
  if (last_slash_pos != std::string::npos) {
    filename = filename.substr(last_slash_pos + 1);
  }

  std::ostringstream logging_stream;

  // Process ID
  logging_stream << base::Process::Current().Pid();

  // Time
  base::Time now = base::Time::Now();
  base::Time::Exploded exploded;
  now.LocalExplode(&exploded);

  char time_buffer[256] = {0, };
  base::strings::SafeSPrintf(time_buffer, "%02d:%02d:%02d",
                             exploded.hour, exploded.minute, exploded.second);
  logging_stream << "/" << time_buffer;

  if (severity_ != LOGGING_ACCESS) {
    // Severity
    logging_stream << "/" << severity_;

    // Logging source
    logging_stream << "/" << filename << "(" << line_ << ")";
  }

  logging_stream << "> ";

  // Message
  logging_stream << stream_.str();

  logging_stream << "\n";

  // Send logging service
  LoggingService* logging_service = LoggingService::GetInstance();
  logging_service->Write(severity_, logging_stream.str());
}

} // namespace stellite
