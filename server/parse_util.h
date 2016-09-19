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

#ifndef STELLITE_SERVER_PARSE_UTIL_H_
#define STELLITE_SERVER_PARSE_UTIL_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/values.h"
#include "net/base/net_export.h"

namespace net {

// JSON -> base::Value
NET_EXPORT std::unique_ptr<base::Value> ParseFromJson(const std::string& json);

// JASON configuration -> base::Value
NET_EXPORT std::unique_ptr<base::Value> ParseFromJsonFile(
  const base::FilePath& config_file);

NET_EXPORT bool StringToUnsignedCharVector(const std::string& source,
  std::vector<uint8_t>* dest);

NET_EXPORT void HeadersToRaw(std::string* headers);

} // Not applicable
#endif // STELLITE_SERVER_PARSE_UTIL_H_
