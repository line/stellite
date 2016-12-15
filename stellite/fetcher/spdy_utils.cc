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

#include "stellite/fetcher/spdy_utils.h"

#include "base/logging.h"
#include "base/strings/string_piece.h"
#include "net/http/http_request_headers.h"

namespace net {

const char* const kForbiddenHttpHeaderFields[] = {
  ":authority",
  ":method",
  ":path",
  ":scheme",
  ":version",
  "method",
  "scheme",
  "version",
};

URLFetcher::RequestType ParseMethod(const SpdyHeaderBlock& spdy_headers,
                                    const SpdyMajorVersion spdy_version) {
  std::string header_key = spdy_version >= SPDY3 ? ":method" : "method";
  std::string method = ParseHeader(header_key, spdy_headers, spdy_version);
  std::transform(method.begin(), method.end(), method.begin(), ::toupper);

  if (method == "GET") {
    return URLFetcher::RequestType::GET;
  } else if (method == "POST") {
    return URLFetcher::RequestType::POST;
  } else if (method == "HEAD") {
    return URLFetcher::RequestType::HEAD;
  } else if (method == "DELETE") {
    return URLFetcher::RequestType::DELETE_REQUEST;
  } else if (method == "PUT") {
    return URLFetcher::RequestType::PUT;
  } else if (method == "PATCH") {
    return URLFetcher::RequestType::PATCH;
  }

  LOG(ERROR) << "Unknown request method: " << method;
  return URLFetcher::RequestType::GET;
}

std::string ParseHeader(const std::string& header_key,
                        const SpdyHeaderBlock& spdy_headers,
                        const SpdyMajorVersion spdy_version) {
  SpdyHeaderBlock::const_iterator it = spdy_headers.find(header_key);
  if (it == spdy_headers.end()) {
    LOG(ERROR) << "Cannot find an HTTP method: " << header_key;
    return std::string();
  }

  return it->second.as_string();
}

bool ConvertSpdyHeaderToHttpRequest(const SpdyHeaderBlock& spdy_headers,
                                    const SpdyMajorVersion spdy_version,
                                    HttpRequestHeaders* request_headers) {
  CHECK(request_headers);
  request_headers->Clear();

  SpdyHeaderBlock::const_iterator it = spdy_headers.begin();
  while (it != spdy_headers.end()) {
    bool valid_header = true;
    for (size_t i = 0; i < arraysize(kForbiddenHttpHeaderFields); ++i) {
      if (it->first == kForbiddenHttpHeaderFields[i]) {
        valid_header = false;
        break;
      }
    }

    if (!valid_header) {
      ++it;
      continue;
    }

    base::StringPiece key(it->first);
    base::StringPiece value(it->second);

    if (key.size() && key[0] == ':') {
      key = key.substr(1);
    }

    request_headers->SetHeader(key, value);
    ++it;
  }

  return true;
}


} // namespace net

