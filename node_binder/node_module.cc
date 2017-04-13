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

#include <sstream>

#include "base/at_exit.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_number_conversions.h"
#include "net/quic/core/crypto/crypto_server_config_protobuf.h"
#include "node/node.h"
#include "node_binder/node_http_fetcher_wrap.h"
#include "node_binder/node_message_pump_manager.h"
#include "node_binder/node_quic_server_stream_wrap.h"
#include "node_binder/node_quic_server_wrap.h"
#include "node_binder/quic_server_config_util.h"
#include "node_binder/v8/converter.h"

using v8::Converter;
using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::String;
using v8::Value;

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

void NODE_MODULE_EXPORT GenerateQuicServerConfig(
    const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();

  int expiry_seconds = 60 * 60 * 24 * 180; // default expire date 6 months
  if (args[0]->IsNumber()) {
    if (!Converter<int>::FromV8(isolate, args[0], &expiry_seconds)) {
      LOG(WARNING) << "expiry seconds overflow of integer range so "
          << "expire_seconds are setting a default value" << expiry_seconds;
    }
  }

  bool use_p256 = false;
  if (args[1]->IsBoolean()) {
    Converter<bool>::FromV8(isolate, args[1], &use_p256);
  }

  bool use_channel_id = false;
  if (args[2]->IsBoolean()) {
    Converter<bool>::FromV8(isolate, args[2], &use_channel_id);
  }

  std::unique_ptr<net::QuicServerConfigProtobuf>
      server_config(stellite::QuicServerConfigUtil::GenerateConfig(
              use_p256, use_channel_id, expiry_seconds));

  std::string encoded_config;
  CHECK(stellite::QuicServerConfigUtil::EncodeConfig(server_config.get(),
                                                     &encoded_config));
  args.GetReturnValue().Set(StringToSymbol(isolate, encoded_config));
}

void NODE_MODULE_EXPORT Init(Local<Object> exports, Local<Object> module) {
  Isolate* isolate = exports->GetIsolate();

  stellite::NodeQuicServerWrap::Init(isolate);
  stellite::NodeQuicServerStreamWrap::Init(isolate);
  stellite::NodeHttpFetcherWrap::Init(isolate);

  NODE_SET_METHOD(exports, "createQuicServer", &CreateQuicServer);
  NODE_SET_METHOD(exports, "createHttpFetcher", &CreateHttpFetcher);
  NODE_SET_METHOD(exports, "setMinLogLevel", &SetMinLogLevel);
  NODE_SET_METHOD(exports, "generateQuicServerConfig", &GenerateQuicServerConfig);
}

NODE_MODULE(stellite, Init);
