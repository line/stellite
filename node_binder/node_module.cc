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

#include "base/at_exit.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "node/node.h"
#include "node_binder/node_http_fetcher_wrap.h"
#include "node_binder/node_message_pump_manager.h"
#include "node_binder/node_quic_server_stream_wrap.h"
#include "node_binder/node_quic_server_wrap.h"
#include "node_binder/v8/converter.h"

using v8::FunctionCallbackInfo;
using v8::Local;
using v8::Object;
using v8::Value;
using v8::Isolate;
using v8::Converter;

namespace {

// TODO(@snibug): change global member to Lazy<ThreadLocalPointer> initialize
base::AtExitManager global_at_exit_manager;
base::MessageLoopForIO global_message_loop;

stellite::NodeMessagePumpManager node_message_loop_manager;

}  // anonymous namespace

void NODE_MODULE_EXPORT CreateQuicServer(
    const FunctionCallbackInfo<Value>& args) {
  stellite::NodeQuicServerWrap::NewInstance(args);
}

void NODE_MODULE_EXPORT CreateHttpFetcher(
    const FunctionCallbackInfo<Value>& args) {
  stellite::NodeHttpFetcherWrap::NewInstance(args);
}

void NODE_MODULE_EXPORT SetMinLogLevel(
    const FunctionCallbackInfo<Value>& args) {
  if (!args[0]->IsNumber()) {
    return;
  }

  int logging_level = 0;
  Isolate* isolate = args.GetIsolate();
  Converter<int>::FromV8(isolate, args[0], &logging_level);
  logging::SetMinLogLevel(logging_level);
}

void NODE_MODULE_EXPORT Init(Local<Object> exports, Local<Object> module) {
  Isolate* isolate = exports->GetIsolate();

  stellite::NodeQuicServerWrap::Init(isolate);
  stellite::NodeQuicServerStreamWrap::Init(isolate);
  stellite::NodeHttpFetcherWrap::Init(isolate);

  NODE_SET_METHOD(exports, "createQuicServer", &CreateQuicServer);
  NODE_SET_METHOD(exports, "createHttpFetcher", &CreateHttpFetcher);
  NODE_SET_METHOD(exports, "setMinLogLevel", &SetMinLogLevel);
}

NODE_MODULE(stellite, Init);
