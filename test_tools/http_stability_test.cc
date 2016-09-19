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

#include "base/command_line.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/synchronization/waitable_event.h"
#include "base/test/test_timeouts.h"
#include "base/time/time.h"
#include "net/base/ip_endpoint.h"
#include "net/dns/mock_host_resolver.h"
#include "stellite/include/http_client.h"
#include "stellite/include/http_client_context.h"
#include "stellite/include/http_request.h"
#include "stellite/include/http_response.h"
#include "stellite/test/http_test_server.h"
#include "stellite/test/quic_mock_server.h"

const char* help_message =
  "Usage: http_stability_test [options]\n"
  "\n"
  "Options:\n"
  "-h, --help     Show this help mesage and exit\n"
  "--count        Specify the test count\n"
  "--protocol     Specify the HTTP protocol (HTTP,HTTPS,SPDY,HTTP2,QUIC)\n";

const int kMaxMultiplexedRequestCount = 1000;

class HttpStabilityTest : public HttpResponseDelegate {
 public:
  HttpStabilityTest(int test_count, HttpTestServer::ServerType type)
      : on_test_complete_(true, false),
        test_count_(test_count),
        complete_count_(0),
        send_count_(0),
        server_type_(type),
        test_start_time_(base::TimeTicks::Now()) {
  }

  ~HttpStabilityTest() override {}

  void OnHttpResponse(const HttpResponse& response,
                      const char* data, size_t len) override {
    ++complete_count_;
    while (send_count_ - complete_count_ < kMaxMultiplexedRequestCount &&
           complete_count_ < test_count_ && send_count_ < test_count_) {
      SendRequest();
    }

    if (complete_count_ == test_count_) {
      on_test_complete_.Signal();
    }
  }

  void OnHttpStream(const HttpResponse& response,
                    const char* data, size_t len) override {
  }

  void OnHttpError(int error_code, const std::string& error_message) override {
  }

  void WaitForTerminate() {
    on_test_complete_.Wait();
  }

  void SendRequest() {
    HttpRequest request;
    request.method = "GET";
    if (server_type_ == HttpTestServer::TYPE_HTTP) {
      request.url = "http://example.com:4430";
    } else {
      request.url = "https://example.com:4430";
    }
    client_->Request(request, this);

    ++send_count_;
  }

  void Report() {
    base::TimeDelta diff = base::TimeTicks::Now() - test_start_time_;
    LOG(INFO) << "Total time: " << diff.InMilliseconds() << "ms";
    LOG(INFO) << "Total request: " << test_count_;
    LOG(INFO) << "Total response: " << complete_count_;
    LOG(INFO) << "Request per second: " << complete_count_ / diff.InSecondsF();
  }

  void set_client(HttpClient* client) {
    client_ = client;
  }

 private:
  HttpClient* client_;

  base::WaitableEvent on_test_complete_;
  const int test_count_;
  int complete_count_;
  int send_count_;

  HttpTestServer::ServerType server_type_;

  base::TimeTicks test_start_time_;
};

int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  base::CommandLine* line = base::CommandLine::ForCurrentProcess();

  if (line->HasSwitch("h") || line->HasSwitch("help")) {
    LOG(ERROR) << help_message;
    exit(0);
  }

  if (!line->HasSwitch("protocol")) {
    LOG(ERROR) << "--protocol is not specified";
    exit(1);
  }

  if (!line->HasSwitch("count")) {
    LOG(ERROR) << "--count is not specified";
    exit(1);
  }

  TestTimeouts::Initialize();

  HttpTestServer::Params server_params;
  server_params.port = 4430;

  HttpClientContext::Params client_params;
  std::string protocol = line->GetSwitchValueASCII("protocol");

  if (protocol == "http") {
    server_params.server_type = HttpTestServer::TYPE_HTTP;
  } else if (protocol == "https") {
    server_params.server_type = HttpTestServer::TYPE_HTTPS;
  } else if (protocol == "spdy") {
    client_params.using_spdy = true;
    server_params.server_type = HttpTestServer::TYPE_SPDY31;
  } else if (protocol == "http2") {
    client_params.using_http2 = true;
    server_params.server_type = HttpTestServer::TYPE_HTTP2;
  } else if (protocol == "quic") {
    client_params.using_quic = true;
    client_params.origin_to_force_quic_on = "example.com:4430";
  } else {
    LOG(ERROR) << help_message;
    LOG(ERROR) << "Unknown protocol";
    exit(1);
  }

  int test_count = 0;
  if (!base::StringToInt(line->GetSwitchValueASCII("count"),
                         &test_count)) {
    LOG(ERROR) << "--count must be an integer";
    exit(1);
  }

  // Setting LINE beta to a local host
  scoped_refptr<net::RuleBasedHostResolverProc> host_resolver_proc =
      new net::RuleBasedHostResolverProc(NULL);

  net::ScopedDefaultHostResolverProc scoped_host_resolver_proc;
  scoped_host_resolver_proc.Init(host_resolver_proc.get());
  host_resolver_proc->AddRule("example.com", "127.0.0.1");

  // Initialize HTTP server
  scoped_ptr<HttpTestServer> http_server(new HttpTestServer(server_params));
  http_server->Start();

  // Initialize QUIC server
  scoped_ptr<QuicMockServer> quic_server(new QuicMockServer());
  quic_server->Start(4430);

  // Initialize client
  scoped_ptr<HttpClientContext> client_context(
      new HttpClientContext(client_params));
  client_context->Initialize();

  HttpStabilityTest test(test_count, server_params.server_type);

  // Start testing
  HttpClient* client(client_context->CreateHttpClient());
  test.set_client(client);
  test.SendRequest();

  test.WaitForTerminate();
  test.Report();

  client_context->ReleaseHttpClient(client);
  http_server->Shutdown();
  quic_server->Shutdown();

  return 0;
}
