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

#ifndef STELLITE_INCLUDE_LOGGING_H_
#define STELLITE_INCLUDE_LOGGING_H_

#include <string>
#include <sstream>

#include "stellite_export.h"

namespace stellite {

typedef int LogSeverity;

const LogSeverity LOGGING_INFO = 0;
const LogSeverity LOGGING_WARNING = 1;
const LogSeverity LOGGING_ERROR = 2;
const LogSeverity LOGGING_FATAL = 3;

#define STELLITE_LOG_EX_INFO(ClassName, ...) \
  stellite::ClassName(__FILE__, __LINE__, stellite::LOGGING_INFO, ##__VA_ARGS__)
#define STELLITE_LOG_EX_WARNING(ClassName, ...) \
  stellite::ClassName(__FILE__, __LINE__, stellite::LOGGING_WARNING, ##__VA_ARGS__)
#define STELLITE_LOG_EX_ERROR(ClassName, ...) \
  stellite::ClassName(__FILE__, __LINE__, stellite::LOGGING_ERROR, ##__VA_ARGS__)
#define STELLITE_LOG_EX_FATAL(ClassName, ...) \
  stellite::ClassName(__FILE__, __LINE__, stellite::LOGGING_FATAL, ##__VA_ARGS__)

#define STELLITE_LOG_INFO \
  STELLITE_LOG_EX_INFO(LogMessage)
#define STELLITE_LOG_WARNING \
  STELLITE_LOG_EX_WARNING(LogMessage)
#define STELLITE_LOG_ERROR \
  STELLITE_LOG_EX_ERROR(LogMessage)
#define STELLITE_LOG_FATAL \
  STELLITE_LOG_EX_FATAL(LogMessage)

#define STELLITE_LOG_IS_ON(severity) \
  ((::stellite::LOGGING_ ## severity) >= ::stellite::GetMinLogLevel())

#define STELLITE_LAZY_STREAM(stream, condition) \
  !(condition) ? (void) 0 : stellite::LogMessageVoidify() & (stream)

#define STELLITE_LOG_STREAM(severity) STELLITE_LOG_ ## severity.stream()

#define LLOG(severity) STELLITE_LAZY_STREAM(STELLITE_LOG_STREAM(severity),\
  STELLITE_LOG_IS_ON(severity))

STELLITE_EXPORT int GetMinLogLevel();
STELLITE_EXPORT void SetMinLogLevel(int level);

class STELLITE_EXPORT LogMessage {
 public:
  LogMessage(const char* file, int line, LogSeverity severity);
  virtual ~LogMessage();

  std::ostream& stream();

 private:
  class LogMessageImpl;
  LogMessageImpl* impl_;
};

class STELLITE_EXPORT LogMessageVoidify {
 public:
  LogMessageVoidify() {}
  void operator&(std::ostream&) {}
};

} // namespace stellite

#endif // STELLITE_INCLUDE_LOGGING_H_
