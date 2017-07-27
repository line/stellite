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

#include "stellite/stub/client_binder.h"

#include <map>
#include <memory>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/synchronization/waitable_event.h"
#include "stellite/include/http_client.h"
#include "stellite/include/http_client_context.h"
#include "stellite/include/http_request.h"
#include "stellite/include/http_response.h"

namespace stellite {

class BinderResponse {
 public:
  BinderResponse(int request_id, const HttpResponse& response,
                 const char* data, size_t len):
      request_id_(request_id),
      connection_info_(response.connection_info),
      content_length_(len),
      response_code_(response.response_code),
      content_body_(data, len),
      connection_info_desc_(response.connection_info_desc),
      response_headers_(new HttpResponseHeader(response.headers)) {
  }

  ~BinderResponse() {}

  int request_id() {
    return request_id_;
  }

  int response_code() {
    return response_code_;
  }

  int content_length() {
    return content_length_;
  }

  const char* body() {
    return content_body_.c_str();
  }

  const char* raw_headers() {
    if (raw_headers_.size() == 0) {
      raw_headers_.assign(response_headers_->raw_headers());
    }
    return raw_headers_.c_str();
  }

  bool has_key(const char* key) {
    CHECK(key);
    return response_headers_->HasHeader(key);
  }

  const char* get_header(const char* key) {
    CHECK(key);
    size_t iter = 0;
    if (!response_headers_->EnumerateHeader(&iter, key, &cache_header_value_)) {
      return nullptr;
    }
    return cache_header_value_.c_str();
  }

  int header_count() {
    int ret = 0;
    size_t iter = 0;
    std::string key, value;
    while (response_headers_->EnumerateHeaderLines(&iter, &key, &value)) {
      ++ret;
    }
    return ret;
  }

  const char* header_key(int pos) {
    std::string value;
    size_t iter = 0;
    for (int count = 0;
         response_headers_->EnumerateHeaderLines(&iter, &cache_header_key_,
                                                 &value);
         ++count) {
      if (pos == count) {
        return cache_header_key_.c_str();
      }
    }
    return nullptr;
  }

  const char* header_value(int pos) {
    std::string name;
    size_t iter = 0;
    for (int count = 0;
         response_headers_->EnumerateHeaderLines(&iter, &name,
                                                 &cache_header_value_);
         ++count) {
      if (pos == count) {
        return cache_header_value_.c_str();
      }
    }
    return nullptr;
  }

  int connection_info() {
    return connection_info_;
  }

  const char* connection_info_desc() {
    return connection_info_desc_.c_str();
  }

 private:
  int request_id_;
  int connection_info_;
  int content_length_;
  int response_code_;
  std::string content_body_;
  std::string raw_headers_;
  std::string connection_info_desc_;

  std::string cache_header_key_;
  std::string cache_header_value_;

  std::unique_ptr<HttpResponseHeader> response_headers_;
};

class BinderHttpClientContext : public HttpClientContext,
                                public HttpResponseDelegate {
 public:
  BinderHttpClientContext(const Params& params)
      : HttpClientContext(params),
        wait_object_(base::WaitableEvent::ResetPolicy::MANUAL,
                     base::WaitableEvent::InitialState::NOT_SIGNALED) {
  }

  ~BinderHttpClientContext() override {}

  void OnHttpResponse(int request_id, const HttpResponse& response,
                      const char* data, size_t len) override {
    std::unique_ptr<BinderResponse> binder_response(
        new BinderResponse(request_id, response, data, len));
    response_map_.insert(std::make_pair(request_id,
                                        std::move(binder_response)));
    wait_object_.Signal();
  }

  void OnHttpStream(int request_id, const HttpResponse& response,
                    const char* data, size_t len, bool is_last) override {
    wait_object_.Signal();
  }

  void OnHttpError(int request_id, int error_code,
                   const std::string& error_message) override {
    LOG(ERROR) << "on http error: " << error_code;
    LOG(ERROR) << "message: " << error_message;
    wait_object_.Signal();
  }

  BinderResponse* WaitForResponse(int request_id) {
    while (!wait_object_.IsSignaled()) {
      wait_object_.Wait();
    }

    BinderResponseMap::iterator it = response_map_.find(request_id);
    if (it == response_map_.end()) {
      LOG(ERROR) << "response is null error: " << request_id;
      return nullptr;
    }
    return it->second.get();
  }

  void ResetEvent() {
    wait_object_.Reset();
  }

  void ReleaseResponse(int request_id) {
    BinderResponseMap::iterator it = response_map_.find(request_id);
    if (it == response_map_.end()) {
      return;
    }
    response_map_.erase(it);
  }

 private:
  typedef std::map<int, std::unique_ptr<BinderResponse>> BinderResponseMap;
  BinderResponseMap response_map_;

  base::WaitableEvent wait_object_;

  DISALLOW_COPY_AND_ASSIGN(BinderHttpClientContext);
};

} // namespace stellite

using stellite::BinderResponse;
using stellite::HttpClient;
using stellite::BinderHttpClientContext;
using stellite::HttpRequest;

extern "C" {

void* new_context() {
  BinderHttpClientContext::Params params;
  params.using_quic = false;
  params.using_http2 = true;

  std::unique_ptr<BinderHttpClientContext> context(
      new BinderHttpClientContext(params));
  if (!context->Initialize()) {
    return NULL;
  }

  return context.release();
}

void* new_context_with_quic() {
  BinderHttpClientContext::Params params;
  params.using_quic = true;
  params.using_http2 = true;

  std::unique_ptr<BinderHttpClientContext> context(
      new BinderHttpClientContext(params));
  if (!context->Initialize()) {
    return NULL;
  }

  return context.release();
}

void* new_context_with_quic_host(const char* host, bool disk_cache) {
  CHECK(host);

  BinderHttpClientContext::Params params;
  params.using_quic = true;
  params.using_http2 = true;

  // TODO(@snibug) remove quic server config cache
  //params.using_quic_disk_cache = disk_cache;
  params.origins_to_force_quic_on.push_back(std::string(host));

  std::unique_ptr<BinderHttpClientContext> context(
      new BinderHttpClientContext(params));
  if (!context->Initialize()) {
    return NULL;
  }

  return context.release();
}

bool release_context(void* raw_context) {
  CHECK(raw_context);

  std::unique_ptr<BinderHttpClientContext> context;
  context.reset(reinterpret_cast<BinderHttpClientContext*>(raw_context));
  return context->TearDown();
}

void* new_client(void* raw_context) {
  CHECK(raw_context);
  BinderHttpClientContext* context =
      reinterpret_cast<BinderHttpClientContext*>(raw_context);
  return context->CreateHttpClient(context);
}

bool release_client(void* raw_context, void* raw_client) {
  CHECK(raw_context);
  CHECK(raw_client);

  BinderHttpClientContext* context;
  context = reinterpret_cast<BinderHttpClientContext*>(raw_context);

  HttpClient* client;
  client = reinterpret_cast<HttpClient*>(raw_client);
  context->ReleaseHttpClient(client);
  return true;
}

void* get(void* raw_context, void* raw_client, char* url) {
  CHECK(raw_client);
  CHECK(url);

  HttpClient* client =
      reinterpret_cast<HttpClient*>(raw_client);

  HttpRequest request;
  request.url = url;
  request.request_type = stellite::HttpRequest::GET;

  BinderHttpClientContext* context =
      reinterpret_cast<BinderHttpClientContext*>(raw_context);
  context->ResetEvent();

  int request_id = client->Request(request);
  return context->WaitForResponse(request_id);
}

void* post(void* raw_context, void* raw_client, char* url, char* body) {
  CHECK(raw_client);
  CHECK(url);
  CHECK(body);

  HttpClient* client = reinterpret_cast<HttpClient*>(
      raw_client);

  HttpRequest request;
  request.url = url;
  request.request_type = stellite::HttpRequest::POST;

  std::string ascii_body(body);
  request.upload_stream.write(ascii_body.c_str(), ascii_body.size());

  BinderHttpClientContext* context =
      reinterpret_cast<BinderHttpClientContext*>(raw_context);
  context->ResetEvent();

  int request_id = client->Request(request);

  return context->WaitForResponse(request_id);
}

void release_response(void* raw_context, void* raw_response) {
  CHECK(raw_response);
  BinderResponse* response = reinterpret_cast<BinderResponse*>(raw_response);

  BinderHttpClientContext* context =
      reinterpret_cast<BinderHttpClientContext*>(raw_context);
  int request_id = response->request_id();
  context->ReleaseResponse(request_id);
}

int response_code(void* raw_response) {
  CHECK(raw_response);

  BinderResponse* response =reinterpret_cast<BinderResponse*>(raw_response);
  return response->response_code();
}

const char* response_body(void* raw_response) {
  CHECK(raw_response);

  BinderResponse* response = reinterpret_cast<BinderResponse*>(raw_response);
  return response->body();
}

const char* raw_headers(void* raw_response) {
  CHECK(raw_response);

  BinderResponse* response = reinterpret_cast<BinderResponse*>(raw_response);
  return response->raw_headers();
}

const char* get_header_value(void* raw_response, int pos) {
  CHECK(raw_response);

  BinderResponse* response = reinterpret_cast<BinderResponse*>(raw_response);
  return response->header_value(pos);
}

const char* header_value(void* raw_response, int pos) {
  CHECK(raw_response);

  BinderResponse* response = reinterpret_cast<BinderResponse*>(raw_response);
  return response->header_value(pos);
}

int get_header_count(void* raw_response) {
  CHECK(raw_response);

  BinderResponse* response = reinterpret_cast<BinderResponse*>(raw_response);
  return response->header_count();
}

const char* get_header_key(void* raw_response, int pos) {
  CHECK(raw_response);

  BinderResponse* response = reinterpret_cast<BinderResponse*>(raw_response);
  return response->header_key(pos);
}

const char* get_header_body(void* raw_response, int pos) {
  CHECK(raw_response);

  BinderResponse* response = reinterpret_cast<BinderResponse*>(raw_response);
  return response->header_value(pos);
}

int get_connection_info(void* raw_response) {
  CHECK(raw_response);

  BinderResponse* response = reinterpret_cast<BinderResponse*>(raw_response);
  return response->connection_info();
}

const char* get_connection_info_desc(void* raw_response) {
  CHECK(raw_response);

  BinderResponse* response = reinterpret_cast<BinderResponse*>(raw_response);
  return response->connection_info_desc();
}



} // extern "C"
