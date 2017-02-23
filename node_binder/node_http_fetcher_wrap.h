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

#ifndef NODE_BINDER_NODE_HTTP_FETCHER_WRAP_H_
#define NODE_BINDER_NODE_HTTP_FETCHER_WRAP_H_

#include "node/node.h"
#include "node/node_object_wrap.h"
#include "stellite/fetcher/http_fetcher_task.h"
#include "stellite/include/stellite_export.h"

namespace stellite {
class NodeHttpFetcher;

class STELLITE_EXPORT NodeHttpFetcherWrap
    : public node::ObjectWrap,
      public HttpFetcherTask::Visitor {
 public:
  static void Init(v8::Isolate* isolate);
  static void NewInstance(const v8::FunctionCallbackInfo<v8::Value>& args);

  void OnTaskComplete(
      int request_id,
      const net::URLFetcher* source,
      const net::HttpResponseInfo* response_info) override;

  void OnTaskHeader(
      int request_id,
      const net::URLFetcher* source,
      const net::HttpResponseInfo* response_info) override;

  void OnTaskStream(
      int request_id,
      const char* data, size_t len, bool fin) override;

  void OnTaskError(
      int request_id,
      const net::URLFetcher* source,
      int error_code) override;

 private:
  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Request(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void AppendChunkToUpload(
      const v8::FunctionCallbackInfo<v8::Value>& args);

  static v8::Persistent<v8::Function> constructor_;

  NodeHttpFetcherWrap(v8::Isolate* isolate,
                      v8::Local<v8::Function> incoming_response);
  ~NodeHttpFetcherWrap() override;

  void OnRequestComplete(int request_id);

  // handle IncomingResponse that defined _incoming_response.js
  v8::Local<v8::Value> CreateIncomingResponse(int request_id);
  v8::Local<v8::Value> GetIncomingResponse(int request_id);
  void ReleaseIncomingResponse(int request_id);

  base::WeakPtr<HttpFetcherTask::Visitor> GetWeakPtr();
  NodeHttpFetcher* fetcher() { return http_fetcher_.get(); }

  v8::Isolate* isolate_;
  v8::UniquePersistent<v8::Object> response_map_;
  v8::UniquePersistent<v8::Function> incoming_response_;

  std::unique_ptr<NodeHttpFetcher> http_fetcher_;

  base::WeakPtrFactory<NodeHttpFetcherWrap> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(NodeHttpFetcherWrap);
};

}  // namespace stellite

#endif  // NODE_BINDER_NODE_HTTP_FETCHER_WRAP_H_
