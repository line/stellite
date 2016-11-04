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

#include "stellite/logging/file_switch.h"

namespace stellite {

const char kDefaultFileName[] = "quic_server.log";

FileSwitch::FileSwitch()
    : filename_(kDefaultFileName) {
}

FileSwitch::~FileSwitch() {
}

bool FileSwitch::CheckName(const std::string& filename) {
  return filename_ == filename;
}

const std::string& FileSwitch::CurrentName() {
  return filename_;
}

} // namespace stellite
