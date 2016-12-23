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

#include "stellite/server/parse_util.h"

#include "base/files/file_util.h"
#include "base/json/json_reader.h"

namespace net {

std::unique_ptr<base::Value> ParseFromJson(const std::string& json) {
  std::unique_ptr<base::Value> value(
      base::JSONReader::Read(json, base::JSON_ALLOW_TRAILING_COMMAS));
  if (!value || !value->IsType(base::Value::TYPE_DICTIONARY)) {
    LOG(ERROR) << "Failed to parse config from JSON";
    return nullptr;
  }

  return value;
}

std::unique_ptr<base::Value> ParseFromJsonFile(
    const base::FilePath& config_file) {
  std::string json;
  if (!base::ReadFileToString(config_file, &json)) {
    LOG(ERROR) << "Failed to read " << config_file.value();
    return nullptr;
  }

  return ParseFromJson(json);
}

bool StringToUnsignedCharVector(const std::string& source,
                                std::vector<uint8_t>* dest) {
  if (!dest) {
    return false;
  }

  dest->resize(source.size(), '\0');
  for (size_t i = 0; i < source.size(); ++i) {
    (*dest)[i] = static_cast<unsigned char>(source[i]);
  }
  return true;
}

void HeadersToRaw(std::string* headers) {
  std::replace(headers->begin(), headers->end(), '\n', '\0');
  if (!headers->empty()) {
    headers->push_back('\0');
  }
}

} // namespace net
