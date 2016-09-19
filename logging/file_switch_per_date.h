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

#include <string>

#include "base/time/time.h"
#include "stellite/logging/file_switch.h"

namespace stellite {

class FileSwitchPerDate : public FileSwitch {
 public:
  explicit FileSwitchPerDate(const std::string& service_key);

  ~FileSwitchPerDate() override;

  // Check whether the log file name is valid or not
  bool CheckName(const std::string& filename) override;

  const std::string& CurrentName() override;

 private:
  std::string service_key_;

  // Cache last year, last month, and the previous day
  base::Time::Exploded last_exploded_;

  DISALLOW_COPY_AND_ASSIGN(FileSwitchPerDate);
};

} // namespace stellite
