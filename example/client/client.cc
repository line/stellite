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

#include <iostream>
#include <sstream>

#include "http_client.h"
#include "http_client_context.h"
#include "http_request.h"
#include "http_response.h"

namespace stellite {

class HttpCallback : public HttpResponseDelegate {
 public:
  HttpCallback()
      : HttpResponseDelegate() {
  }

  ~HttpCallback() override {}

  void OnHttpResponse(const HttpResponse& response, const char* data,
                      size_t len) override {
    std::cout << "CONNECTION_INFO: " << response.connection_info << std::endl
              << "  0: unknown" << std::endl << "  1: http1" << std::endl
              << "  2: spdy2" << std::endl << "  3: spdy3" << std::endl
              << "  4: http2" << std::endl << "  5: quic/spdy3" << std::endl
              << "  6: http2/draft-14" << std::endl << "  7: http2/draft-15"
              << std::endl;

    std::cout << "CACHE: " << response.was_cached << std::endl
              << "PROXY: " << response.was_fetched_via_proxy << std::endl
              << "NETWORK ACCESSED: " << response.network_accessed
              << std::endl << "SPDY: " << response.was_fetched_via_spdy
              << std::endl << "NPN_NEGOTIATED: "
              << response.was_npn_negotiated << std::endl;

    std::cout << "HEADERS:" << std::endl;
    void* it = nullptr;
    std::string name, value;
    while (response.headers->EnumerateHeaderLines(&it, &name, &value)) {
      std::cout << "  " << name << ": " << value << std::endl;
    }

    std::cout << "DATA:" << std::endl;
    std::cout << std::string(data, len);
  }

  void OnHttpStream(const HttpResponse& response, const char* data, size_t len)
      override {
    std::cout << "ON http stream response " << std::string(data, len);
    std::cout << std::endl;
  }

  void OnHttpError(int error_code, const std::string& error_message) override {
    std::cout << "OnHttpError error_code: " << error_code << std::endl;
    std::cout << "error_message: " << error_message << std::endl;
  }
};

} // namespace stellite

// namespace stellite

bool CheckMethod(const std::string& method) {
  std::string lower(method);
  std::transform(lower.begin(), lower.end(), lower.begin(), ::toupper);
  if (lower == "GET") {
    return true;
  }
  else if (lower == "POST") {
    return true;
  }
  else if (lower == "PUT") {
    return true;
  }
  else if (lower == "DELETE") {
    return true;
  }
  return false;
}

int main() {
  stellite::HttpClientContext::Params params;
  params.using_spdy = true;
  params.using_http2 = true;

  // Support QUIC
  params.using_quic = true;
  params.using_quic_disk_cache = true;

  sp<stellite::HttpCallback> callback = new stellite::HttpCallback();

  stellite::HttpClientContext context(params);
  context.Initialize();
  stellite::HttpClient* client = context.CreateHttpClient();

  while (true) {
    stellite::HttpRequest request;

    while (true) {
      std::getline(std::cin, request.url);
      if (request.url.size()) {
        break;
      }
    }
    // TODO(@snibug): Check URL validation

    std::cout << "method: ";
    if (std::getline(std::cin, request.method) &&
        !CheckMethod(request.method)) {
      request.method = "GET";
    }

    // Body upload
    while (true) {
      std::cout << "content type: ";
      std::string content_type;
      if (!std::getline(std::cin, content_type) || content_type.size() == 0) {
        break;
      }

      std::string body;
      if (!std::getline(std::cin, body)) {
        break;
      }

      request.headers->SetHeader("Content-Type", content_type);
      request.upload_stream.write(body.c_str(), body.size());
    }

    // Send request
    client->Request(request, callback);
  }

  context.ReleaseHttpClient(client);
  context.TearDown();

  return 0;
}
