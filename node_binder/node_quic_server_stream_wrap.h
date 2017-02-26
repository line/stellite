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

#ifndef NODE_BINDER_NODE_QUIC_SERVER_STREAM_WRAP_H_
#define NODE_BINDER_NODE_QUIC_SERVER_STREAM_WRAP_H_

#include "base/macros.h"
#include "node/node.h"
#include "node/node_object_wrap.h"
#include "node_binder/node_quic_server_stream.h"
#include "stellite/include/stellite_export.h"

namespace stellite {

class NodeQuicServerStreamWrap;

class STELLITE_EXPORT NodeQuicServerStreamWrap
    : public node::ObjectWrap,
      public NodeQuicServerStream::Visitor {
 public:
  static void Init(v8::Isolate* isolate);
  static v8::Local<v8::Value> NewInstance(v8::Isolate* isolate,
                                          NodeQuicServerStream* stream);

  // implements NodeQuicServerStream::Visitor interface
  void OnHeadersAvailable(NodeQuicServerStream* stream,
                          const net::SpdyHeaderBlock& headers,
                          bool fin) override;

  void OnDataAvailable(NodeQuicServerStream* stream,
                       const char* buffer, size_t len, bool fin) override;

 private:
  NodeQuicServerStreamWrap(NodeQuicServerStream* stream);
  ~NodeQuicServerStreamWrap() override;

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void WriteData(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void WriteHeaders(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void WriteTrailers(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void SetHeadersAvailableCallback(
      const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetDataAvailableCallback(
      const v8::FunctionCallbackInfo<v8::Value>& args);

  static v8::Persistent<v8::Function> constructor_;

  NodeQuicServerStream* stream_;
  v8::UniquePersistent<v8::Function> on_headers_available_callback_;
  v8::UniquePersistent<v8::Function> on_data_available_callback_;

  DISALLOW_COPY_AND_ASSIGN(NodeQuicServerStreamWrap);
};

}  // namespace stellite

#endif  // NODE_BINDER_NODE_QUIC_SERVER_STREAM_WRAP_H_
