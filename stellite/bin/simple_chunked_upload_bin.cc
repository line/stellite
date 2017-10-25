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

#include <iostream>
#include <memory>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "stellite/include/http_client.h"
#include "stellite/include/http_client_context.h"
#include "stellite/include/http_request.h"
#include "stellite/include/http_response.h"

namespace stellite {

class HttpCallback : public HttpResponseDelegate {
 public:
  HttpCallback();
  ~HttpCallback() override;

  void OnHttpResponse(int requeset_id, const HttpResponse& response,
                      const char* data, size_t len) override;

  void OnHttpStream(int request_id, const HttpResponse& response,
                    const char* data, size_t len, bool fin) override;

  void OnHttpError(int request_id, int error_code,
                   const std::string& error_message) override;

};

HttpCallback::HttpCallback() {}
HttpCallback::~HttpCallback() {}

void HttpCallback::OnHttpResponse(int requeset_id, const HttpResponse& response,
                                  const char* data, size_t len) {
  LOG(INFO)
      << "CONNECTION_INFO: " << response.connection_info << std::endl
      << "0: unknown" << std::endl << "1: http1" << std::endl
      << "2: spdy2" << std::endl << "3: spdy3" << std::endl
      << "4: http2" << std::endl << "5: quic/spdy3" << std::endl
      << "6: http2/draft-14" << std::endl << "7: http2/draft-15"
      << std::endl;

  LOG(INFO) << std::endl << "  CACHE: " << response.was_cached << std::endl
      << "  PROXY: " << response.was_fetched_via_proxy << std::endl
      << "  NETWORK ACCESSED: " << response.network_accessed
      << std::endl << "  SPDY: " << response.was_fetched_via_spdy
      << std::endl << "  NPN_NEGOTIATED: "
      << response.was_alpn_negotiated << std::endl;

  LOG(INFO) << std::endl << "response code: " << response.response_code
      << std::endl << "mime_type: " << response.mime_type
      << std::endl << "charset: " << response.charset << std::endl;

  size_t iter = 0;
  std::string name, value;
  while (response.headers.EnumerateHeaderLines(&iter, &name, &value)) {
    LOG(INFO) << name << ": " << value;
  }

  LOG(INFO) << std::endl << std::endl << "DATA:" << std::string(data, len);
}

void HttpCallback::OnHttpStream(int request_id, const HttpResponse& response,
                                const char* data, size_t len, bool fin) {
  LOG(INFO) << "ON http stream response " << std::string(data, len);
}

void HttpCallback::OnHttpError(int request_id, int error_code,
                               const std::string& error_message) {
  LOG(INFO) << "OnHttpError error_code: " << error_code;
  LOG(INFO) << "error_message: " << error_message;
}

} // namespace stellite

void PrintHelpMessage() {
  const char* help_str = "Usage: http_client [options]\n"
      "\n"
      "Options:\n"
      "-h, --help                  Show this help message and exit\n"
      "--url                       specify the url for chunked upload\n"
      "--quic-host                 specify the quic direct request host\n";
  std::cout << help_str;
}

int main(int argc, char *argv[]) {
  // The url for chunked upload
  std::string FLAGS_url = "";
  std::string FLAGS_quic_host = "";

  base::CommandLine::Init(argc, argv);
  base::CommandLine* line = base::CommandLine::ForCurrentProcess();

  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  CHECK(logging::InitLogging(settings));

  if (line->HasSwitch("h") || line->HasSwitch("help")) {
    PrintHelpMessage();
    exit(0);
  }

  if (!line->HasSwitch("url")) {
    PrintHelpMessage();
    exit(0);
  }

  if (line->HasSwitch("quic-host")) {
    FLAGS_quic_host = line->GetSwitchValueASCII("quic-host");
  }

  base::AtExitManager at_exit_manager;

  stellite::HttpClientContext::Params params;
  params.using_quic = true;
  params.using_http2 = true;

  if (FLAGS_quic_host.size()) {
    params.origins_to_force_quic_on.push_back(FLAGS_quic_host);
  }

  std::unique_ptr<stellite::HttpCallback> callback(
      new stellite::HttpCallback());

  stellite::HttpClientContext context(params);
  context.Initialize();

  stellite::HttpRequest chunked_upload_request;
  chunked_upload_request.url = line->GetSwitchValueASCII("url");
  chunked_upload_request.is_chunked_upload = true;
  chunked_upload_request.is_stream_response = true;

  stellite::HttpClient* client = context.CreateHttpClient(callback.get());
  int request_id = client->Request(chunked_upload_request, 0);
  CHECK(request_id >= 0);

  while (true) {
    std::string chunk;
    std::getline(std::cin, chunk);
    client->AppendChunkToUpload(request_id, chunk, false);
  }

  context.ReleaseHttpClient(client);
  context.TearDown();

  return 0;
}
