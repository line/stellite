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

#ifndef STELLITE_LOGGING_LOGGING_H_
#define STELLITE_LOGGING_LOGGING_H_

#include <sstream>

namespace stellite {

typedef size_t LogSeverity;

const LogSeverity LOGGING_INFO = 0;
const LogSeverity LOGGING_ACCESS = 1;
const LogSeverity LOGGING_ERROR = 2;
const LogSeverity LOGGING_DEBUG = 3;
const LogSeverity LOGGING_END = 4;

#define LOG_ACCESS(ClassName) \
stellite::ClassName(stellite::LOGGING_ACCESS, __FILE__, __LINE__)

#define LOG_ERROR(ClassName) \
stellite::ClassName(stellite::LOGGING_ERROR, __FILE__, __LINE__)

#define LOG_DEBUG(ClassName) \
stellite::ClassName(stellite::LOGGING_DEBUG, __FILE__, __LINE__)

#define LOG_INFO(ClassName) \
stellite::ClassName(stellite::LOGGING_INFO, __FILE__, __LINE__)

#define FILE_LOG_MESSAGE_ACCESS LOG_ACCESS(FileLogMessage)
#define FILE_LOG_MESSAGE_ERROR  LOG_ERROR(FileLogMessage)
#define FILE_LOG_MESSAGE_DEBUG  LOG_DEBUG(FileLogMessage)
#define FILE_LOG_MESSAGE_INFO   LOG_INFO(FileLogMessage)

#define FILE_LOG_STREAM(severity) FILE_LOG_MESSAGE_ ## severity.stream()

#define FLOG(severity) FILE_LOG_STREAM(severity)

class FileLogMessage {
 public:
  FileLogMessage(LogSeverity severity, const char* file, int line);

  ~FileLogMessage();

  std::ostream& stream() {
    return stream_;
  }

 private:
  LogSeverity severity_;
  const char* file_;
  int line_;
  std::ostringstream stream_;
};

} // namespace stellite


#endif // STELLITE_LOGGING_LOGGING_H_
