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

#ifndef STELLITE_LOGGING_LOG_FILE_WRITER_H_
#define STELLITE_LOGGING_LOG_FILE_WRITER_H_

#include <memory>
#include <queue>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"

namespace stellite {
class FileSwitch;
class LogFile;
class LogFileFactory;

class LogFileWriter {
 public:
  explicit LogFileWriter(LogFileFactory* file_factory,
                         FileSwitch* file_switch);
  virtual ~LogFileWriter();

  void PostStream(const std::string& message);

 private:
  void StartWriting();

  void OnWriteComplete(int result);

  // Current log file name will be generated from |log_switch_|
  std::string filename_;

  // Logging queue
  std::queue<std::string> log_queue_;

  // Queue lock
  base::Lock lock_;

  // Current log file name validation
  std::unique_ptr<FileSwitch> file_switch_;

  // Logging file target
  std::unique_ptr<LogFile> log_file_;

  // Keep track of whether a read is currently in-flight
  bool write_pending_;

  // The number of iterations of the write loop that have been
  // completed synchronously
  int synchronous_write_count_;

  LogFileFactory* log_file_factory_;

  base::WeakPtrFactory<LogFileWriter> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(LogFileWriter);
};

} // namespace stellite

#endif // STELLITE_LOGGING_LOG_FILE_WRITER_H_
