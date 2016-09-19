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

#ifndef STELLITE_LOGGING_LOG_FILE_FACTORY_DEFAULT_H_
#define STELLITE_LOGGING_LOG_FILE_FACTORY_DEFAULT_H_

#include <string>

#include "base/files/file_path.h"
#include "stellite/logging/log_file_factory.h"

namespace stellite {
class LogFile;

class LogFileFactoryDefault : public LogFileFactory {
 public:
  explicit LogFileFactoryDefault(const base::FilePath& base_dir);

  ~LogFileFactoryDefault() override;

  LogFile* CreateLogFile(const std::string& filename) override;

 private:
  // Default logging directory
  base::FilePath base_dir_;

  DISALLOW_COPY_AND_ASSIGN(LogFileFactoryDefault);
};

} // namespace stellite

#endif // STELLITE_LOGGING_LOG_FILE_FACTORY_DEFAULT_H_
