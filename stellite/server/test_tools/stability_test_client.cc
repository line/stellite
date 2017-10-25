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

#include <signal.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>

#include <algorithm>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/threading/platform_thread.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"
#include "stellite/include/http_client.h"
#include "stellite/include/http_client_context.h"
#include "stellite/include/http_request.h"
#include "stellite/include/http_response.h"
#include "url/gurl.h"

bool completeTest = false;
int requestPerSecond = 10;
int testTotalSecond = 60 * 60;
int total_req_count = 0;
int total_res_count = 0;
int total_res_error_count = 0;
pid_t myPid;
std::string currentConnectionInfoDesc;
std::string prevConnectionInfoDesc;
std::string requestURL;
struct timeval testStartTime;

// 1,000,000 usec == 1000 msec == 1 sec
int64 timerIntervalUsec = (1 * 1000 * 1000) / requestPerSecond;

const char* help_message =
  "Usage:\n"
  "stability_test_client [options]"
  "\n"
  "options:\n"
  "-h, --help                 Show this help message and text\n"
  "--url                      Request URL (Example: http://example.com)\n"
  "--tps                      Request per second\n"
  "--second                   Total request seconds\n"
  "--quic                     HTTP request using QUIC protocol\n"
  "--http2                    HTTP request using HTTP/2 protocol\n";

void CheckTestTime() {
  struct timeval curTime;
  gettimeofday(&curTime, NULL);
  if(curTime.tv_sec - testStartTime.tv_sec > testTotalSecond) {
    if((total_res_count + total_res_error_count) == total_req_count) {
      completeTest = true;
    }
  }
}

class HttpCallback : public stellite::HttpResponseDelegate {
 public:
  HttpCallback()
      : HttpResponseDelegate() {
  }

  ~HttpCallback() override {}

  void OnHttpResponse(const stellite::HttpResponse& response, const char* data,
                      size_t len) override {
    total_res_count += 1;

    CheckTestTime();
  }

  void OnHttpStream(const stellite::HttpResponse& response,
                    const char* data, size_t len)
      override {
    total_res_count += 1;

    CheckTestTime();
  }

  void OnHttpError(int error_code, const std::string& error_message) override {
    total_res_error_count += 1;

    CheckTestTime();
  }
};


stellite::HttpClientContext* pHttpClientContext;
stellite::HttpClient* pHttpClient;


void SendHttpRequest(std::string url) {
  if(completeTest) {
    return;
  }

  stellite::HttpRequest request;

  request.url = url;
  request.method = "GET";

  sp<HttpCallback> callback = new HttpCallback();

  pHttpClient->Request(request, callback);

  total_req_count+= 1;
}

double currentCPUUsage;
double currentMemUsage;
int currentVSZ;
int max_virtual_memory = std::numeric_limits<int>::min();
int min_virtual_memory = std::numeric_limits<int>::max();

double total_cpu_usage = 0.0;
double total_mem_usage = 0.0;


void RefreshCurrentCPUMemUsage() {
  std::ostringstream command_stream;
  command_stream << "ps aux";
  command_stream << " | awk '{print $2,$3,$4,$5,$11}'";
  command_stream << " | grep " << myPid;

  char buffer[256] = {0, };
  FILE* out_file = popen(command_stream.str().c_str(), "r");
  fgets(buffer, 256, out_file);
  pclose(out_file);

  std::ostringstream out_stream;
  out_stream << buffer;

  std::stringstream filter_stream(out_stream.str());

  std::string token;
  filter_stream >> token;  // Token: process ID

  filter_stream >> token;  // Token: CPU usage
  currentCPUUsage = std::atof(token.c_str());
  total_cpu_usage += currentCPUUsage;

  filter_stream >> token;  // Token: memory usage
  currentMemUsage = std::atof(token.c_str());
  total_mem_usage += currentMemUsage;

  filter_stream >> token;  // Token: VSZ (virtual memory size)
  currentVSZ = std::atoi(token.c_str());

  max_virtual_memory = std::max(max_virtual_memory, currentVSZ);
  min_virtual_memory = std::min(min_virtual_memory, currentVSZ);
}

std::string GetCurrentDateTimeStr() {
  std::string retStr;

  time_t rawTime;
  time(&rawTime);

  retStr = ctime(&rawTime);
  retStr = retStr.substr(0, retStr.length() - 1);

  return retStr;
}

void PrintCurrentInfo() {
  std::ostringstream outStringStream;

  outStringStream << GetCurrentDateTimeStr();

  outStringStream << ", " << total_req_count;
  outStringStream << ", " << total_res_count;
  outStringStream << ", " << total_res_error_count;

  outStringStream << ", " << currentCPUUsage;
  outStringStream << ", " << currentMemUsage;
  outStringStream << ", " << currentVSZ;

  std::cout << outStringStream.str() << std::endl;
}

void PrintReport(const std::string& protocol) {
  std::cout << std::endl << "# Test report #" << std::endl;

  std::cout << "protocol: " << protocol << std::endl;

  std::cout << "total request: " << total_req_count << std::endl;
  std::cout << "total response: " << total_res_count << std::endl;

  int count = std::min(total_req_count, total_res_count);
  std::cout << "average cpu usage: " << total_cpu_usage / count << std::endl;
  std::cout << "average mem usage: " << total_mem_usage / count << std::endl;

  std::cout << "max mem usage: " << max_virtual_memory << "bytes" << std::endl;
  std::cout << "min mem usage: " << min_virtual_memory << "bytes" << std::endl;
  int mid_virtual_memory = (max_virtual_memory + min_virtual_memory) / 2;
  std::cout << "mid mem usage: " << mid_virtual_memory << "bytes" << std::endl;
}

void TimerHandler (int signum) {
  PrintCurrentInfo();
  SendHttpRequest(requestURL);
}

void* GetCpuMemUsageThreadFunc(void* argument) {
  while(1) {
    RefreshCurrentCPUMemUsage();

    if(completeTest)
      break;

    base::PlatformThread::Sleep(
        base::TimeDelta::FromMicroseconds(timerIntervalUsec));
  }

  return NULL;
}

int main(int argc, char** argv) {
  base::CommandLine::Init(argc, argv);
  base::CommandLine* line = base::CommandLine::ForCurrentProcess();

  if (!line->HasSwitch("url")) {
    LOG(ERROR) << "Error: URL options are required";
    std::cout << help_message;
    exit(1);
  }
  std::string raw_url = line->GetSwitchValueASCII("url");

  GURL url(raw_url);
  if (!url.is_valid()) {
    LOG(ERROR) << "Invalid URL: " << raw_url;
    exit(1);
  }
  requestURL = url.spec();

  if (!line->HasSwitch("tps")) {
    LOG(ERROR) << "Error: TPS options are required";
    std::cout << help_message;
    exit(1);
  }
  requestPerSecond = atoi(line->GetSwitchValueASCII("tps").c_str());

  if (!line->HasSwitch("second")) {
    LOG(ERROR) << "Error: Second options are required";
    std::cout << help_message;
    exit(1);
  }
  testTotalSecond = atoi(line->GetSwitchValueASCII("second").c_str());

  // 1,000,000 usec == 1000 msec == 1 sec
  timerIntervalUsec = (1 * 1000 * 1000) / requestPerSecond;

  stellite::HttpClientContext::Params params;
  std::string protocol;
  if (line->HasSwitch("quic")) {
    params.using_quic = true;

    if (!url.SchemeIs("https")) {
      LOG(ERROR) << "HTTPS scheme is required to test with QUIC";
      exit(1);
    }

    std::stringstream quic_origin;
    quic_origin << url.host() << ":" << url.EffectiveIntPort();
    params.origin_to_force_quic_on = quic_origin.str();

    std::cout << "QUIC Host info: " << params.origin_to_force_quic_on;
    std::cout << std::endl;

    protocol = "quic";
  } else if (line->HasSwitch("http2")) {
    params.using_http2 = true;
    protocol = "http2";
  } else {
    if (url.SchemeIs("https")) {
      protocol = "https";
    } else if (url.SchemeIs("http")) {
      protocol = "http";
    }
  }

  pHttpClientContext = new stellite::HttpClientContext(params);
  pHttpClientContext->Initialize();
  pHttpClient = pHttpClientContext->CreateHttpClient();

  myPid = getpid();

  std::cout << "PID: " << myPid << std::endl;
  std::cout << "Request URL: " << requestURL << std::endl;
  std::cout << "Request Per Second: " << requestPerSecond << std::endl;
  std::cout << "Running time(sec): " << testTotalSecond << std::endl;
  std::cout << "datetime, req, res, error res, cpu, mem(physical), ";
  std::cout << "mem(virtual KiB)" << std::endl;

  int intervalSecond;
  int64 restUsec;

  if (timerIntervalUsec >= (1 * 1000 * 1000)) {
    intervalSecond = timerIntervalUsec / (1 * 1000 * 1000);
    restUsec = timerIntervalUsec - (intervalSecond * 1 * 1000 * 1000);
  } else {
    intervalSecond = 0;
    restUsec = timerIntervalUsec;
  }

  gettimeofday(&testStartTime, NULL);

  pthread_t tid;
  pthread_create(&tid, NULL, &GetCpuMemUsageThreadFunc, NULL);

  while (1) {
    TimerHandler(0);

    base::PlatformThread::Sleep(
        base::TimeDelta::FromSeconds(intervalSecond));

    base::PlatformThread::Sleep(
        base::TimeDelta::FromMicroseconds(restUsec));

    if (completeTest)
      break;
  }

  pthread_join(tid, NULL);

  pHttpClientContext->ReleaseHttpClient(pHttpClient);
  pHttpClientContext->TearDown();

  PrintReport(protocol);

  return 0;
}
