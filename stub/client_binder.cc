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
#include "stellite/include/ref.h"

namespace stellite {

class ResponseDelegate : public HttpResponseDelegate {
 public:
  ResponseDelegate()
      : response_code_(-1),
        content_length_(0),
        body_(),
        response_event_(true, false) {
  }
  ~ResponseDelegate() override {}

  void OnHttpResponse(const HttpResponse& response, const char* data,
                      size_t len) override {
    response_code_ = response.response_code;
    content_length_ = len;
    body_.assign(data, len);
    response_headers_ = response.headers;

    connection_info_ = response.connection_info;
    connection_info_desc_ = response.connection_info_desc;

    response_event_.Signal();
  }

  void OnHttpStream(const HttpResponse& response, const char* data,
                    size_t len) override {
    response_event_.Signal();
  }

  void OnHttpError(int error_code, const std::string& error_message) override {
    LOG(ERROR) << "error code: " << error_code;
    LOG(ERROR) << "message: " << error_message;
    response_event_.Signal();
  }

  void WaitForResponse() {
    if (response_event_.IsSignaled()) {
      return;
    }
    response_event_.Wait();
  }

  void TimedWaitForResponse(int millisec) {
    if (response_event_.IsSignaled()) {
      return;
    }
    response_event_.TimedWait(base::TimeDelta::FromMilliseconds(millisec));
  }

  int response_code_;
  int content_length_;
  std::string body_;

  int connection_info_;
  std::string connection_info_desc_;

  sp<HttpResponseHeader> response_headers_;
  base::WaitableEvent response_event_;
};

class Response {
 public:
  Response() : delegate_(new ResponseDelegate()) {
  }

  ~Response() {
  }

  int response_code() {
    return delegate_->response_code_;
  }

  int content_length() {
    return delegate_->content_length_;
  }

  const char* body() {
    raw_body_.assign(delegate_->body_);
    return raw_body_.c_str();
  }

  const char* raw_headers() {
    raw_headers_.assign(delegate_->response_headers_->raw_headers());
    return raw_headers_.c_str();
  }

  bool has_key(const char* key) {
    CHECK(key);
    return delegate_->response_headers_->HasHeader(key);
  }

  const char* get_header(const char* key) {
    CHECK(key);
    size_t iter = 0;
    if (!delegate_->response_headers_->EnumerateHeader(&iter, key, &value_)) {
      return nullptr;
    }
    return value_.c_str();
  }

  int header_count() {
    int ret = 0;
    size_t iter = 0;
    std::string k, v;
    while (delegate_->response_headers_->EnumerateHeaderLines(&iter, &k, &v)) {
      ++ret;
    }
    return ret;
  }

  const char* header_name(int pos) {
    std::string value;
    size_t iter = 0;
    for (int count = 0;
         delegate_->response_headers_->EnumerateHeaderLines(&iter, &name_,
                                                            &value);
         ++count) {
      if (pos == count) {
        return name_.c_str();
      }
    }
    return nullptr;
  }

  const char* header_value(int pos) {
    std::string name;
    size_t iter = 0;
    for (int count = 0;
         delegate_->response_headers_->EnumerateHeaderLines(&iter, &name,
                                                            &value_);
         ++count) {
      if (pos == count) {
        return value_.c_str();
      }
    }
    return nullptr;
  }

  void Wait() {
    delegate_->WaitForResponse();
  }

  void TimedWait(int millisec) {
    delegate_->TimedWaitForResponse(millisec);
  }

  sp<ResponseDelegate> delegate() {
    return delegate_;
  }

  int connection_info() {
    return delegate_->connection_info_;
  }

  const char* connection_info_desc() {
    return delegate_->connection_info_desc_.c_str();
  }

 private:
  sp<ResponseDelegate> delegate_;
  std::string raw_headers_;
  std::string raw_body_;
  std::string name_;
  std::string value_;
};

} // namespace stellite

extern "C" {

void* new_context() {
  stellite::HttpClientContext::Params params;
  params.using_quic = false;
  params.using_spdy = true;
  params.using_http2 = true;

  std::unique_ptr<stellite::HttpClientContext> context(
      new stellite::HttpClientContext(params));
  if (!context->Initialize()) {
    return NULL;
  }

  return context.release();
}

void* new_context_with_quic() {
  stellite::HttpClientContext::Params params;
  params.using_quic = true;
  params.using_spdy = true;
  params.using_http2 = true;

  std::unique_ptr<stellite::HttpClientContext> context(
      new stellite::HttpClientContext(params));
  if (!context->Initialize()) {
    return NULL;
  }

  return context.release();
}

void* new_context_with_quic_host(const char* host, bool disk_cache) {
  CHECK(host);

  stellite::HttpClientContext::Params params;
  params.using_quic = true;
  params.using_spdy = true;
  params.using_http2 = true;
  params.using_quic_disk_cache = disk_cache;
  params.origin_to_force_quic_on = std::string(host);

  std::unique_ptr<stellite::HttpClientContext> context(
      new stellite::HttpClientContext(params));
  if (!context->Initialize()) {
    return NULL;
  }

  return context.release();
}

bool release_context(void* raw_context) {
  CHECK(raw_context);

  std::unique_ptr<stellite::HttpClientContext> context;
  context.reset(reinterpret_cast<stellite::HttpClientContext*>(raw_context));
  return context->TearDown();
}

void* new_client(void* raw_context) {
  CHECK(raw_context);

  stellite::HttpClientContext* context;
  context = reinterpret_cast<stellite::HttpClientContext*>(raw_context);
  return context->CreateHttpClient();
}

bool release_client(void* raw_context, void* raw_client) {
  CHECK(raw_context);
  CHECK(raw_client);

  stellite::HttpClientContext* context;
  context = reinterpret_cast<stellite::HttpClientContext*>(raw_context);

  stellite::HttpClient* client;
  client = reinterpret_cast<stellite::HttpClient*>(raw_client);
  context->ReleaseHttpClient(client);
  return true;
}

void* get(void* raw_client, char* url) {
  CHECK(raw_client);
  CHECK(url);

  stellite::HttpClient* client = reinterpret_cast<stellite::HttpClient*>(
      raw_client);
  stellite::Response* response = new stellite::Response();

  stellite::HttpRequest request;
  request.url = url;
  request.method = "GET";
  client->Request(request, response->delegate());

  response->Wait();
  return response;
}

void* post(void* raw_client, char* url, char* body) {
  CHECK(raw_client);
  CHECK(url);
  CHECK(body);

  stellite::HttpClient* client = reinterpret_cast<stellite::HttpClient*>(
      raw_client);
  stellite::Response* response = new stellite::Response();

  stellite::HttpRequest request;
  request.url = url;
  request.method = "POST";

  std::string ascii_body(body);
  request.upload_stream.write(ascii_body.c_str(), ascii_body.size());
  client->Request(request, response->delegate());
  response->Wait();
  return response;
}

void release_response(void* raw_response) {
  CHECK(raw_response);

  stellite::Response* response = reinterpret_cast<stellite::Response*>(
      raw_response);
  delete response;
}

int response_code(void* raw_response) {
  CHECK(raw_response);

  stellite::Response* response = reinterpret_cast<stellite::Response*>(
      raw_response);
  return response->response_code();
}

int header_count(void* raw_response) {
  CHECK(raw_response);

  stellite::Response* response = reinterpret_cast<stellite::Response*>(
      raw_response);
  return response->header_count();
}

const char* response_body(void* raw_response) {
  CHECK(raw_response);

  stellite::Response* response = reinterpret_cast<stellite::Response*>(
      raw_response);

  return response->body();
}

const char* raw_headers(void* raw_response) {
  stellite::Response* response = reinterpret_cast<stellite::Response*>(
      raw_response);

  return response->raw_headers();
}

const char* header_name(void* raw_response, int pos) {
  CHECK(raw_response);

  stellite::Response* response = reinterpret_cast<stellite::Response*>(
      raw_response);
  return response->header_name(pos);
}

const char* header_value(void* raw_response, int pos) {
  CHECK(raw_response);

  stellite::Response* response = reinterpret_cast<stellite::Response*>(
      raw_response);
  return response->header_name(pos);
}

int get_connection_info(void* raw_response) {
  CHECK(raw_response);

  stellite::Response* response = reinterpret_cast<stellite::Response*>(
      raw_response);
  return response->connection_info();
}

const char* get_connection_info_desc(void* raw_response) {
  CHECK(raw_response);

  stellite::Response* response = reinterpret_cast<stellite::Response*>(
      raw_response);
  return response->connection_info_desc();
}

} // extern "C"
