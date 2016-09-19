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

#ifndef STELLITE_LOGGING_FILE_SWITCH_H_
#define STELLITE_LOGGING_FILE_SWITCH_H_

#include <string>

#include "base/macros.h"

namespace stellite {

class FileSwitch {
 public:
  FileSwitch();

  virtual ~FileSwitch();

  // Check whether the log file name is valid or not
  virtual bool CheckName(const std::string& filename);

  virtual const std::string& CurrentName();

 protected:
  std::string filename_;

 private:
  DISALLOW_COPY_AND_ASSIGN(FileSwitch);
};

} // namespace stellite

#endif
