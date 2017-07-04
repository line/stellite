// Copyright 2017 LINE Corporation
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

#include "node_binder/node_http_fetcher_wrap.h"

#include "base/logging.h"
#include "base/threading/thread_task_runner_handle.h"
#include "node/node_buffer.h"
#include "node_binder/node_http_fetcher.h"
#include "node_binder/node_message_pump_manager.h"
#include "node_binder/v8/converter.h"

namespace stellite {

using v8::Context;
using v8::Converter;
using v8::EscapableHandleScope;
using v8::Exception;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::HandleScope;
using v8::Isolate;
using v8::Local;
using v8::Number;
using v8::Object;
using v8::Persistent;
using v8::String;
using v8::Value;
using v8::V8ToString;
using v8::Boolean;
using v8::Null;

namespace {

const char kAppendChunkToUpload[] = "appendChunkToUpload";
const char kData[] = "data";
const char kEmit[] = "emit";
const char kError[] = "error";
const char kHeaders[] = "headers";
const char kIsChunkedUpload[] = "is_chunked_upload";
const char kIsStopOnRedirect[] = "is_stop_on_redirect";
const char kIsStreamResponse[] = "is_stream_response";
const char kMaxRetriesOn5xx[] = "max_retries_on_5xx";
const char kMaxRetriesOnNetworkChange[] = "max_retries_on_network_change";
const char kMethod[] = "method";
const char kNodeHttpFetcher[] = "NodeHttpFetcher";
const char kPayload[] = "payload";
const char kRemoveAllListeners[] = "removeAllListeners";
const char kRequest[] = "request";
const char kResponse[] = "response";
const char kTimeout[] = "timeout";
const char kUrl[] = "url";

const int kDefaultTimeout = 60 * 1000; // 60 second

}  // namespace anonymouse

NodeHttpFetcherWrap::NodeHttpFetcherWrap(Isolate* isolate,
                                         Local<Function> incoming_response)
    : isolate_(isolate),
      response_map_(isolate, Object::New(isolate)),
      incoming_response_(isolate, incoming_response),
      http_fetcher_(new NodeHttpFetcher()),
      weak_factory_(this) {
  http_fetcher_->InitDefaultOptions();
}

NodeHttpFetcherWrap::~NodeHttpFetcherWrap() {}

void NodeHttpFetcherWrap::OnTaskComplete(
    int request_id,
    const net::URLFetcher* source,
    const net::HttpResponseInfo* response_info) {
  HandleScope scope(isolate_);
  Local<Context> context(isolate_->GetCurrentContext());
  Context::Scope context_scope(context);

  Local<Object> incoming_response =
      Local<Object>::Cast(GetIncomingResponse(request_id));
  DCHECK(incoming_response->IsObject());

  Local<String> emit_callback_key = String::NewFromUtf8(isolate_, kEmit);
  DCHECK(incoming_response->Has(context, emit_callback_key).FromJust());

  Local<Function> emit =
      Local<Function>::Cast(
          incoming_response->Get(context, emit_callback_key)
          .ToLocalChecked());

  if (!source || !response_info) {
    Local<Value> args[] = {
      String::NewFromUtf8(isolate_, kError),
      Number::New(isolate_, -1),
    };

    // javascript: incoming_response.emit('error', -1)
    emit->Call(incoming_response, arraysize(args), args);

    OnRequestComplete(request_id);
    return;
  }

  const net::URLRequestStatus& status = source->GetStatus();
  if (status.status() == net::URLRequestStatus::FAILED) {
    Local<Value> args[] = {
      String::NewFromUtf8(isolate_, kError),
      Number::New(isolate_, status.error()),
      Converter<net::HttpResponseHeaders>::ToV8(
          isolate_,
          *(response_info->headers.get())),
    };

    // javascript: incoming_response.emit('error', error_code, headers);
    emit->Call(incoming_response, arraysize(args), args);
    OnRequestComplete(request_id);
  }

  // successed
  Local<Value> args[] = {
    String::NewFromUtf8(isolate_, kResponse),
    Converter<net::HttpResponseHeaders>::ToV8(
        isolate_,
        *(response_info->headers.get())),
    Null(isolate_),
  };

  std::string payload;
  source->GetResponseAsString(&payload);

  char* buffer = nullptr;
  if (payload.size()) {
    buffer = static_cast<char*>(malloc(payload.size()));
    memcpy(buffer, payload.data(), payload.size());
    args[2] =
        node::Buffer::New(isolate_, buffer, payload.size())
        .ToLocalChecked();
  }

  // javascript: incoming_resposne.emit('response', headers, body)
  emit->Call(incoming_response, arraysize(args), args);

  OnRequestComplete(request_id);
}

void NodeHttpFetcherWrap::OnTaskHeader(
    int request_id,
    const net::URLFetcher* source,
    const net::HttpResponseInfo* response_info) {
  HandleScope scope(isolate_);
  Local<Context> context(isolate_->GetCurrentContext());
  Context::Scope context_scope(context);

  Local<Object> incoming_response =
      Local<Object>::Cast(GetIncomingResponse(request_id));
  DCHECK(incoming_response->IsObject());

  Local<String> emit_callback_key = String::NewFromUtf8(isolate_, kEmit);
  DCHECK(incoming_response->Has(context, emit_callback_key).FromJust());

  Local<Function> emit =
      Local<Function>::Cast(
          incoming_response->Get(context, emit_callback_key)
          .ToLocalChecked());

  Local<Value> args[] = {
    String::NewFromUtf8(isolate_, kHeaders),
    Converter<net::HttpResponseHeaders>::ToV8(
        isolate_,
        *(response_info->headers.get())),
  };

  // javascript: incoming_response.emit('header', headers)
  emit->Call(incoming_response, arraysize(args), args);
}

void NodeHttpFetcherWrap::OnTaskStream(
    int request_id,
    const char* data, size_t len, bool fin) {
  HandleScope scope(isolate_);
  Local<Context> context(isolate_->GetCurrentContext());
  Context::Scope context_scope(context);

  Local<Object> incoming_response =
      Local<Object>::Cast(GetIncomingResponse(request_id));
  DCHECK(incoming_response->IsObject());

  Local<String> emit_callback_key = String::NewFromUtf8(isolate_, kEmit);
  DCHECK(incoming_response->Has(context, emit_callback_key).FromJust());

  Local<Function> emit =
      Local<Function>::Cast(
          incoming_response->Get(context, emit_callback_key)
          .ToLocalChecked());

  char* buffer = static_cast<char*>(malloc(len));
  memcpy(buffer, data, len);

  Local<Value> args[] = {
    String::NewFromUtf8(isolate_, kData),
    node::Buffer::New(isolate_, buffer, len).ToLocalChecked(),
  };

  // javascript: incoming_response.emit('data', stream)
  emit->Call(incoming_response, arraysize(args), args);

  if (fin) {
    OnRequestComplete(request_id);
  }
}

void NodeHttpFetcherWrap::OnTaskError(
    int request_id,
    const net::URLFetcher* source,
    int error_code) {
  HandleScope scope(isolate_);
  Local<Context> context(isolate_->GetCurrentContext());
  Context::Scope context_scope(context);

  Local<Object> incoming_response =
      Local<Object>::Cast(GetIncomingResponse(request_id));
  DCHECK(incoming_response->IsObject());

  Local<String> emit_callback_key = String::NewFromUtf8(isolate_, kEmit);
  DCHECK(incoming_response->Has(context, emit_callback_key).FromJust());

  Local<Function> emit =
      Local<Function>::Cast(
          incoming_response->Get(context, emit_callback_key)
          .ToLocalChecked());

  Local<Value> args[] = {
    String::NewFromUtf8(isolate_, kError),
    Number::New(isolate_, error_code),
  };

  // javascript: incoming_response.emit('error', stream)
  emit->Call(incoming_response, arraysize(args), args);

  OnRequestComplete(request_id);
}

base::WeakPtr<HttpFetcherTask::Visitor> NodeHttpFetcherWrap::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

Local<Value> NodeHttpFetcherWrap::CreateIncomingResponse(int request_id) {
  EscapableHandleScope scope(isolate_);
  Local<Function> incoming_response =
      Local<Function>::New(isolate_, incoming_response_);

  Local<Context> context = isolate_->GetCurrentContext();
  Context::Scope context_scope(context);

  Local<Value> args[] = {
    handle(),
    Number::New(isolate_, request_id),
  };

  Local<Object> response =
      incoming_response->NewInstance(context, arraysize(args), args)
      .ToLocalChecked();

  Local<Object> response_map = Local<Object>::New(isolate_, response_map_);
  Local<Number> request_key = Number::New(isolate_, request_id);
  DCHECK(!response_map->Has(context, request_key).FromJust());

  response_map->Set(context, request_key, response).FromJust();

  return scope.Escape(response);
}

Local<Value> NodeHttpFetcherWrap::GetIncomingResponse(int request_id) {
  EscapableHandleScope scope(isolate_);
  Local<Context> context = isolate_->GetCurrentContext();
  Context::Scope context_scope(context);

  Local<Object> response_map = Local<Object>::New(isolate_, response_map_);
  Local<Number> request_key = Number::New(isolate_, request_id);
  if (!response_map->Has(context, request_key).FromJust()) {
    return scope.Escape(Null(isolate_));
  }

  Local<Object> response =
      Local<Object>::Cast(
          response_map->Get(context, request_key)
          .ToLocalChecked());

  return scope.Escape(response);
}

void NodeHttpFetcherWrap::ReleaseIncomingResponse(int request_id) {
  HandleScope scope(isolate_);
  Local<Context> context = isolate_->GetCurrentContext();
  Context::Scope context_scope (context);

  Local<Object> incoming_response =
      Local<Object>::Cast(GetIncomingResponse(request_id));
  if (incoming_response->IsNull()) {
    return;
  }

  Local<String> remove_listeners_key =
      String::NewFromUtf8(isolate_, kRemoveAllListeners);
  DCHECK(incoming_response->Has(context, remove_listeners_key).FromJust());

  Local<Function> remove_listeners =
      Local<Function>::Cast(
          incoming_response->Get(context, remove_listeners_key)
          .ToLocalChecked());

  // javascript: incoming_response.removeAllListeners()
  remove_listeners->Call(incoming_response, 0, nullptr);

  Local<Object> response_map = Local<Object>::New(isolate_, response_map_);
  Local<Number> request_key = Number::New(isolate_, request_id);
  DCHECK(response_map->Has(context, request_key).FromJust());
  response_map->Delete(context, request_key).FromJust();
}

// static
void NodeHttpFetcherWrap::Init(Isolate* isolate) {
  Local<FunctionTemplate> tmpl = FunctionTemplate::New(isolate, &New);
  tmpl->SetClassName(String::NewFromUtf8(isolate, kNodeHttpFetcher));
  tmpl->InstanceTemplate()->SetInternalFieldCount(1);

  NODE_SET_PROTOTYPE_METHOD(tmpl, kRequest, &Request);
  NODE_SET_PROTOTYPE_METHOD(tmpl, kAppendChunkToUpload, &AppendChunkToUpload);

  constructor_.Reset(isolate, tmpl->GetFunction());
}

// static
void NodeHttpFetcherWrap::NewInstance(
    const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);

  Local<Context> context = isolate->GetCurrentContext();
  Context::Scope context_scope(context);

  // check IncomingResponse constructor function
  if (!args[0]->IsFunction()) {
    isolate->ThrowException(
        Exception::TypeError(
            String::NewFromUtf8(isolate, "invalid IncomingResponse")));
    return;
  }

  Local<Function> constructor = Local<Function>::New(isolate, constructor_);
  Local<Value> argv[] = { args[0] };
  Local<Object> instance =
      constructor->NewInstance(context, arraysize(argv), argv)
      .ToLocalChecked();

  args.GetReturnValue().Set(instance);
}

// static
void NodeHttpFetcherWrap::New(const FunctionCallbackInfo<Value>& args) {
  CHECK(args.IsConstructCall());
  CHECK_EQ(args.Length(), 1);

  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);

  Local<Function> incoming_response = Local<Function>::Cast(args[0]);
  NodeHttpFetcherWrap* obj =
      new NodeHttpFetcherWrap(isolate, incoming_response);
  obj->Wrap(args.This());

  args.GetReturnValue().Set(args.This());
}

// static
void NodeHttpFetcherWrap::Request(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);

  if (!args[0]->IsObject()) {
    isolate->ThrowException(
        Exception::TypeError(
            String::NewFromUtf8(isolate, "invalid request argument")));
    return;
  }
  Local<Object> option = Local<Object>::Cast(args[0]);

  Local<Context> context = isolate->GetCurrentContext();
  Context::Scope context_scope(context);

  HttpRequest request;

  // url
  DCHECK(option->Has(String::NewFromUtf8(isolate, kUrl)));
  request.url = V8ToString(
      option->Get(context, String::NewFromUtf8(isolate, kUrl))
      .ToLocalChecked());

  // method
  Local<Value> method_param = String::NewFromUtf8(isolate, kMethod);
  DCHECK(option->Has(context, method_param).FromJust());
  std::string method =
      V8ToString(option->Get(context, method_param).ToLocalChecked());
  if (method == "GET") {
    request.request_type = HttpRequest::GET;
  } else if (method == "POST") {
    request.request_type = HttpRequest::POST;
  } else if (method == "HEAD") {
    request.request_type = HttpRequest::HEAD;
  } else if (method == "DELETE") {
    request.request_type = HttpRequest::DELETE_REQUEST;
  } else if (method == "PUT") {
    request.request_type = HttpRequest::PUT;
  } else if (method == "PATCH") {
    request.request_type = HttpRequest::PATCH;
  } else  {
    request.request_type = HttpRequest::GET;
  }

  // is_stop_on_redirect
  Local<Value> redirect_param =
      String::NewFromUtf8(isolate, kIsStopOnRedirect);
  DCHECK(option->Has(context, redirect_param).FromJust());
  bool is_stop_on_redirect = false;
  Converter<bool>::FromV8(
      isolate,
      option->Get(context, redirect_param).ToLocalChecked(),
      &is_stop_on_redirect);
  request.is_stop_on_redirect = is_stop_on_redirect;

  // is_chunked_upload
  Local<Value> chunked_param = String::NewFromUtf8(isolate, kIsChunkedUpload);
  DCHECK(option->Has(context, chunked_param).FromJust());
  bool is_chunked_upload = false;
  Converter<bool>::FromV8(
      isolate,
      option->Get(context, chunked_param).ToLocalChecked(),
      &is_chunked_upload);
  request.is_chunked_upload = is_chunked_upload;

  // max_retries_on_5xx
  Local<Value> retries_5xx_param =
      String::NewFromUtf8(isolate, kMaxRetriesOn5xx);
  DCHECK(option->Has(context, retries_5xx_param).FromJust());
  int max_retries_on_5xx = 0;
  Converter<int>::FromV8(
      isolate,
      option->Get(context, retries_5xx_param).ToLocalChecked(),
      &max_retries_on_5xx);
  request.max_retries_on_5xx = max_retries_on_5xx;

  // max_retries_on_network_change
  Local<Value> retries_network_change_param =
      String::NewFromUtf8(isolate, kMaxRetriesOnNetworkChange);
  DCHECK(option->Has(context, retries_network_change_param).FromJust());
  int max_retries_on_network_change = 0;
  Converter<int>::FromV8(
      isolate,
      option->Get(context, retries_network_change_param).ToLocalChecked(),
      &max_retries_on_network_change);
  request.max_retries_on_network_change = max_retries_on_network_change;

  // is_stream_response
  Local<Value> stream_param =
      String::NewFromUtf8(isolate, kIsStreamResponse);
  DCHECK(option->Has(context, stream_param).FromJust());
  bool is_stream_response = false;
  Converter<bool>::FromV8(
      isolate,
      option->Get(context, stream_param).ToLocalChecked(),
      &is_stream_response);
  request.is_stream_response = is_stream_response;

  // timeout
  Local<Value> timeout_param = String::NewFromUtf8(isolate, kTimeout);
  int timeout = kDefaultTimeout;
  DCHECK(option->Has(context, timeout_param).FromJust());
  Converter<int>::FromV8(
      isolate,
      option->Get(context, timeout_param).ToLocalChecked(),
      &timeout);

  // payload
  Local<Value> payload_param = String::NewFromUtf8(isolate, kPayload);
  std::string payload;
  DCHECK(option->Has(context, payload_param).FromJust());
  payload = V8ToString(option->Get(context, payload_param).ToLocalChecked());
  if (payload.size() > 0) {
    request.upload_stream.write(payload.data(), payload.size());
  }

  // add reference count for message pump before request
  NodeMessagePumpManager::current()->AddRef();

  NodeHttpFetcherWrap* obj =
      ObjectWrap::Unwrap<NodeHttpFetcherWrap>(args.Holder());
  NodeHttpFetcher* fetcher = obj->fetcher();

  int request_id = fetcher->Request(request, timeout, obj->GetWeakPtr());

  // create incomfing response
  Local<Value> incoming_response = obj->CreateIncomingResponse(request_id);
  args.GetReturnValue().Set(incoming_response);
}

// static
void NodeHttpFetcherWrap::AppendChunkToUpload(
    const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);

  int request_id = -1;
  if (!Converter<int>::FromV8(isolate, args[0], &request_id)) {
    isolate->ThrowException(
        Exception::TypeError(
            String::NewFromUtf8(isolate, "invalid request_id type")));
    return;
  }

  std::string chunk;
  if (args[1]->IsString()) {
    chunk = V8ToString(args[1]);
  } else if (node::Buffer::HasInstance(args[1])) {
    chunk = std::string(node::Buffer::Data(args[1]),
                        node::Buffer::Length(args[1]));
  } else {
    isolate->ThrowException(
        Exception::TypeError(
            String::NewFromUtf8(isolate, "invalid chunk data type")));
    return;
  }

  bool fin = false;
  if (!Converter<bool>::FromV8(isolate, args[2], &fin)) {
    isolate->ThrowException(
        Exception::TypeError(
            String::NewFromUtf8(isolate, "invalid fin type")));
    return;
  }

  NodeHttpFetcherWrap* obj =
      ObjectWrap::Unwrap<NodeHttpFetcherWrap>(args.Holder());
  NodeHttpFetcher* fetcher = obj->fetcher();

  bool ret = fetcher->AppendChunkToUpload(request_id, chunk, fin);
  args.GetReturnValue().Set(Boolean::New(isolate, ret));
}

void NodeHttpFetcherWrap::OnRequestComplete(int request_id) {
  // remove IncomingResponse reference
  ReleaseIncomingResponse(request_id);

  // manage reference count for message pump
  NodeMessagePumpManager::current()->RemoveRef();
}

Persistent<Function> NodeHttpFetcherWrap::constructor_;

}  // namespace stellite
