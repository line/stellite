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

#ifndef STELLITE_LOGGING_LOGGING_SERVICE_H_
#define STELLITE_LOGGING_LOGGING_SERVICE_H_

#include <map>
#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/singleton.h"
#include "base/threading/thread.h"
#include "stellite/logging/logging.h"

namespace stellite {
class LogFileWriter;
class LogFileFactory;

class LoggingService {
 public:
  static LoggingService* GetInstance();

  bool Initialize(const base::FilePath& log_dir);

  void Shutdown();

  void Write(LogSeverity log_severity, const std::string& message);

 private:
  friend struct base::DefaultSingletonTraits<LoggingService>;

  std::string GetTag(LogSeverity log_severity);

  void OnWrite(LogSeverity log_severity, const std::string& message);

  LoggingService();

  virtual ~LoggingService();

  // A logging process is  running on the IO thread
  std::unique_ptr<base::Thread> logging_thread_;

  std::vector<std::unique_ptr<LogFileWriter>> log_writer_list_;

  std::unique_ptr<LogFileFactory> log_file_factory_;

  DISALLOW_COPY_AND_ASSIGN(LoggingService);
};

} // namespace stellite


#endif // STELLITE_LOGGING_LOGGING_SERVICE_H_
