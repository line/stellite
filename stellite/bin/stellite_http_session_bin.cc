//
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


#include <memory>
#include <iostream>

#include "base/at_exit.h"
#include "base/logging.h"
#include "base/command_line.h"
#include "stellite/include/http_session.h"

namespace stellite {

class ResponseVisitor : public HttpSessionVisitor {
 public:
  ResponseVisitor();
  ~ResponseVisitor() override;

  void OnHttpResponse(int request_id, int response_code,
                      const char* raw_header, size_t header_len,
                      const char* raw_body, size_t body_len,
                      int connection_info) override;

  void OnHttpStream(int request_id, int response_code,
                    const char* raw_header, size_t header_len,
                    const char* chunk, size_t chunk_len,
                    bool fin, int connection_info) override;

  void OnError(int request_id, int error_code,
               const char* reason, size_t len) override;
};

ResponseVisitor::ResponseVisitor() {}
ResponseVisitor::~ResponseVisitor() {}

void ResponseVisitor::OnHttpResponse(int request_id, int response_code,
                                     const char* raw_header, size_t header_len,
                                     const char* raw_body, size_t body_len,
                                     int connection_info) {
  LOG(INFO) << std::endl << "response code: " << response_code;

  std::string headers(raw_header, header_len);
  for (size_t from = 0; from < headers.size();) {
    size_t to = headers.find('\0', from);
    if (to == std::string::npos) {
      break;
    }

    // case "\0\0"
    size_t header_len = to - from;
    if (header_len < 2) {
      break;
    }

    std::string header(headers.substr(from, to - from));
    LOG(INFO) << header;
    from = to + 1;
  }

  std::string body(raw_body, body_len);
  LOG(INFO) << std::endl << body;
}

void ResponseVisitor::OnHttpStream(int request_id, int response_code,
                                   const char* raw_header, size_t header_len,
                                   const char* chunk, size_t chunk_len,
                                   bool fin, int connection_info) {
  LOG(INFO) << std::endl << "response code: " << response_code;

  std::string headers(raw_header, header_len);
  for (size_t from = 0; from < headers.size();) {
    size_t to = headers.find('\0', from);
    if (to == std::string::npos) {
      break;
    }

    // case "\0\0"
    size_t header_len = to - from;
    if (header_len < 2) {
      break;
    }

    std::string header(headers.substr(from, to - from));
    LOG(INFO) << header;
    from = to + 1;
  }

  std::string segment(chunk, chunk_len);
  LOG(INFO) << std::endl << segment;
}

void ResponseVisitor::OnError(int request_id, int error_code,
                              const char* reason, size_t len) {
  LOG(INFO) << "OnError error code: " << error_code
      << "error message: " << std::string(reason, len);
}

}  // namespace stellite

int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);

  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  CHECK(logging::InitLogging(settings));

  std::unique_ptr<stellite::ResponseVisitor> visitor(
      new stellite::ResponseVisitor());
  stellite::HttpSession::Params params;
  params.using_http2 = true;
  params.using_quic = true;
  std::unique_ptr<stellite::HttpSession> session(
      new stellite::HttpSession(params, visitor.get()));

  while (true) {
    std::string command;
    std::getline(std::cin, command);
    session->Request(stellite::HttpSession::HTTP_GET,
                     command.c_str(), command.size(),
                     nullptr, 0, nullptr, 0,
                     /* chunked upload */ false,
                     /* stream_response */ false,
                     /* timeout */ 1000 * 60);
  }

  return 0;
}
