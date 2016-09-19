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

#ifndef QUIC_FETCHER_SPDY_UTILS_H_
#define QUIC_FETCHER_SPDY_UTILS_H_

#include "net/spdy/spdy_header_block.h"
#include "net/spdy/spdy_protocol.h"
#include "net/url_request/url_fetcher.h"

namespace net {

class HttpRequestHeaders;

URLFetcher::RequestType ParseMethod(const SpdyHeaderBlock& spdy_headers,
                                    const SpdyMajorVersion spdy_version);

std::string ParseHeader(const std::string& header_key,
                        const SpdyHeaderBlock& spdy_headers,
                        const SpdyMajorVersion spdy_version);

bool ConvertSpdyHeaderToHttpRequest(const SpdyHeaderBlock& spdy_headers,
                                    const SpdyMajorVersion spdy_version,
                                    HttpRequestHeaders* request_headers);


} // namespace net

#endif
