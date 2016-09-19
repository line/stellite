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

#include "stellite/include/logging.h"

#include "base/logging.h"

namespace stellite {

int GetMinLogLevel() {
  return logging::GetMinLogLevel();
}

void SetMinLogLevel(int level) {
  logging::SetMinLogLevel(level);
}

class LogMessage::LogMessageImpl {
 public:
  LogMessageImpl(const char* file, int line, LogSeverity severity)
      : message_(file, line, severity) {
  }
  ~LogMessageImpl() {}

  std::ostream& stream() {
    return message_.stream();
  }

 private:
  logging::LogMessage message_;

  DISALLOW_COPY_AND_ASSIGN(LogMessageImpl);
};

LogMessage::LogMessage(const char* file, int line, LogSeverity severity)
    : impl_(new LogMessageImpl(file, line, severity)) {
}

LogMessage::~LogMessage() {
  delete impl_;
}

std::ostream& LogMessage::stream() {
  return impl_->stream();
}

} // namespace stellite
