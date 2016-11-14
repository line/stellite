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

#include "stellite/logging/file_switch_per_date.h"

#include "base/strings/safe_sprintf.h"

namespace stellite {

const char kTimebaseLoggingFormat[] = "%04d-%02d-%02d_%s.log";
const int kFileNameBufferSize = 128;

namespace {

bool operator==(const base::Time::Exploded& a, const base::Time::Exploded& b) {
  return a.year == b.year &&
         a.month == b.month &&
         a.day_of_month == b.day_of_month;
}

}

FileSwitchPerDate::FileSwitchPerDate(const std::string& service_key)
    : service_key_(service_key) {
}

FileSwitchPerDate::~FileSwitchPerDate() {
}

bool FileSwitchPerDate::CheckName(const std::string& filename) {
  return CurrentName() == filename;
}

const std::string& FileSwitchPerDate::CurrentName() {
  base::Time now = base::Time::Now();
  base::Time::Exploded exploded;
  now.LocalExplode(&exploded);

  if (exploded == last_exploded_) {
    return filename_;
  }

  last_exploded_.year = exploded.year;
  last_exploded_.month = exploded.month;
  last_exploded_.day_of_month = exploded.day_of_month;

  char buf[kFileNameBufferSize] = {0,};
  int buf_len = base::strings::SafeSPrintf(buf,
                                           kTimebaseLoggingFormat,
                                           exploded.year,
                                           exploded.month,
                                           exploded.day_of_month,
                                           service_key_.c_str());
  filename_.assign(buf, buf_len);
  return filename_;
}

} // namespace stellite
