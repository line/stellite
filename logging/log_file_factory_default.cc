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

#include "stellite/logging/log_file_factory_default.h"

#if defined(OS_POSIX)
#include "stellite/logging/log_file_posix.h"
#endif

namespace stellite {

LogFileFactoryDefault::LogFileFactoryDefault(const base::FilePath& base_dir)
    : base_dir_(base_dir) {
}

LogFileFactoryDefault::~LogFileFactoryDefault() {
}

LogFile* LogFileFactoryDefault::CreateLogFile(const std::string& filename) {
#if defined(OS_POSIX)
  return new LogFilePosix(base_dir_.AppendASCII(filename));
#elif defined(OS_WIN)
  return nullptr;
#endif
}

} // namespace stellite
