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

#include "node_binder/node_quic_server_stream_wrap.h"

#include "base/logging.h"
#include "node/node.h"
#include "node/node_buffer.h"
#include "node/node_object_wrap.h"
#include "node_binder/node_quic_server_stream.h"
#include "node_binder/node_quic_server_wrap.h"
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
using v8::MaybeLocal;
using v8::NewStringType;
using v8::Null;
using v8::Number;
using v8::Object;
using v8::Persistent;
using v8::String;
using v8::Undefined;
using v8::Value;

NodeQuicServerStreamWrap::NodeQuicServerStreamWrap(
    NodeQuicServerStream* stream)
    : stream_(stream) {
}

NodeQuicServerStreamWrap::~NodeQuicServerStreamWrap() {}

void NodeQuicServerStreamWrap::OnHeadersAvailable(
    NodeQuicServerStream* stream,
    const net::SpdyHeaderBlock& headers,
    bool fin) {
  Isolate* isolate(v8::Isolate::GetCurrent());
  HandleScope scope(isolate);

  Local<Function> callback =
      Local<Function>::New(isolate, on_headers_available_callback_);

  Local<Value> args[] = {
    Converter<net::SpdyHeaderBlock>::ToV8(isolate, headers),
    Converter<bool>::ToV8(isolate, fin),
  };

  callback->Call(handle(isolate), arraysize(args), args);
}

void NodeQuicServerStreamWrap::OnDataAvailable(NodeQuicServerStream* stream,
                                               const char* buffer, size_t len,
                                               bool fin) {
  Isolate* isolate(v8::Isolate::GetCurrent());
  HandleScope scope(isolate);

  Local<Function> callback =
      Local<Function>::New(isolate, on_data_available_callback_);

  // copy buffer
  char* base = static_cast<char*>(malloc(len));
  memcpy(base, buffer, len);

  Local<Value> args[] = {
    node::Buffer::New(isolate, base, len).ToLocalChecked(),
    Converter<bool>::ToV8(isolate, fin),
  };

  callback->Call(handle(isolate), arraysize(args), args);
}

// static
void NodeQuicServerStreamWrap::Init(v8::Isolate* isolate) {
  Local<FunctionTemplate> tmpl = FunctionTemplate::New(isolate, &New);
  tmpl->SetClassName(String::NewFromUtf8(isolate, "NodeQuicServerStream"));
  tmpl->InstanceTemplate()->SetInternalFieldCount(1);

  NODE_SET_PROTOTYPE_METHOD(tmpl, "writeData", WriteData);
  NODE_SET_PROTOTYPE_METHOD(tmpl, "writeHeaders", WriteHeaders);
  NODE_SET_PROTOTYPE_METHOD(tmpl, "writeTrailers", WriteTrailers);

  NODE_SET_PROTOTYPE_METHOD(tmpl, "setHeadersAvailableCallback",
                            SetHeadersAvailableCallback);
  NODE_SET_PROTOTYPE_METHOD(tmpl, "setDataAvailableCallback",
                            SetDataAvailableCallback);

  constructor_.Reset(isolate, tmpl->GetFunction());
}

// static
Local<Value> NodeQuicServerStreamWrap::NewInstance(
    v8::Isolate* isolate,
    NodeQuicServerStream* stream) {
  EscapableHandleScope scope(isolate);

  Local<Context> context = isolate->GetCurrentContext();
  Context::Scope context_scope(context);

  Local<Function> constructor = Local<Function>::New(isolate, constructor_);
  Local<Object> instance = constructor->NewInstance(context, 0, nullptr)
      .ToLocalChecked();

  NodeQuicServerStreamWrap* obj = new NodeQuicServerStreamWrap(stream);
  obj->Wrap(instance);
  stream->set_visitor(obj);

  return scope.Escape(instance);
}

// static
void NodeQuicServerStreamWrap::New(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  CHECK(args.IsConstructCall());
  args.GetReturnValue().Set(args.This());
}

// static
void NodeQuicServerStreamWrap::WriteData(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);

  if (!node::Buffer::HasInstance(args[0]) &&
      !args[0]->IsString() &&
      !args[0]->IsNull()) {
    isolate->ThrowException(
        Exception::TypeError(
            String::NewFromUtf8(
                isolate,
                "invalid writeData buffer arguments")));
    return;
  }

  if (!args[1]->IsBoolean()) {
    isolate->ThrowException(
        Exception::TypeError(
            String::NewFromUtf8(isolate, "invalid writeData fin argument")));
    return;
  }

  bool fin = args[1]->BooleanValue();

  std::string data;
  if (node::Buffer::HasInstance(args[0])) {
    data = std::string(node::Buffer::Data(args[0]),
                       node::Buffer::Length(args[0]));
  } else if (args[0]->IsString()) {
    data = std::string(V8ToString(args[0]));
  } else if (!fin) {
    isolate->ThrowException(
        Exception::TypeError(
            String::NewFromUtf8(
                isolate,
                "invalid argument: not finished, no data ...")));
    return;
  }

  NodeQuicServerStreamWrap* obj =
      ObjectWrap::Unwrap<NodeQuicServerStreamWrap>(args.Holder());
  NodeQuicServerStream* stream = obj->stream_;
  DCHECK(stream);
  stream->WriteOrBufferBody(data, fin, nullptr);
}

// static
void NodeQuicServerStreamWrap::WriteHeaders(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);

  if (!args[0]->IsObject() || !args[1]->IsBoolean()) {
    isolate->ThrowException(
        Exception::TypeError(
            String::NewFromUtf8(isolate, "invalid writeHeaders args type")));
    return;
  }

  net::SpdyHeaderBlock headers;
  if (!Converter<net::SpdyHeaderBlock>::FromV8(isolate, args[0], &headers)) {
    isolate->ThrowException(
        Exception::TypeError(
            String::NewFromUtf8(isolate, "invalid writeHeaders headers")));
    return;
  }

  bool fin = args[1]->BooleanValue();

  NodeQuicServerStreamWrap* obj =
      ObjectWrap::Unwrap<NodeQuicServerStreamWrap>(args.Holder());
  NodeQuicServerStream* stream = obj->stream_;
  DCHECK(stream);

  stream->WriteHeaders(std::move(headers), fin, nullptr);
}

// static
void NodeQuicServerStreamWrap::WriteTrailers(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);

  net::SpdyHeaderBlock trailers;
  if (!args[0]->IsObject() ||
      !Converter<net::SpdyHeaderBlock>::FromV8(isolate, args[0], &trailers)) {
    isolate->ThrowException(
        Exception::TypeError(
            String::NewFromUtf8(isolate, "invalid writeTrailers args type")));
    return;
  }

  NodeQuicServerStreamWrap* obj =
      ObjectWrap::Unwrap<NodeQuicServerStreamWrap>(args.Holder());
  NodeQuicServerStream* stream = obj->stream_;
  DCHECK(stream);

  stream->WriteTrailers(std::move(trailers), nullptr);
}

// static
void NodeQuicServerStreamWrap::SetHeadersAvailableCallback(
      const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);

  if (!args[0]->IsFunction()) {
    isolate->ThrowException(
        Exception::TypeError(
            String::NewFromUtf8(isolate,
                                "invalid setHeaderAvailableCallback args")));
    return;
  }

  NodeQuicServerStreamWrap* obj =
      ObjectWrap::Unwrap<NodeQuicServerStreamWrap>(args.Holder());
  obj->on_headers_available_callback_.Reset(
      isolate,
      Local<Function>::Cast(args[0]));
}

// static
void NodeQuicServerStreamWrap::SetDataAvailableCallback(
      const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate(args.GetIsolate());
  HandleScope scope(isolate);

  if (!args[0]->IsFunction()) {
    isolate->ThrowException(
        Exception::TypeError(
            String::NewFromUtf8(isolate,
                                "invalid setDataAvailableCallback args")));
    return;
  }

  NodeQuicServerStreamWrap* obj =
      ObjectWrap::Unwrap<NodeQuicServerStreamWrap>(args.Holder());
  obj->on_data_available_callback_.Reset(
      isolate,
      Local<Function>::Cast(args[0]));
}

// static
v8::Persistent<v8::Function> NodeQuicServerStreamWrap::constructor_;

}  // namespace stellite
