//
// 2017 writen by snibug@gmail.com
//

#include "node_binder/node_quic_server_wrap.h"

#include "net/base/net_errors.h"
#include "net/quic/core/spdy_utils.h"
#include "node/node_buffer.h"
#include "node_binder/node_quic_server.h"
#include "node_binder/node_quic_server_session.h"
#include "node_binder/node_quic_server_stream_wrap.h"
#include "node_binder/v8/converter.h"
#include "url/gurl.h"

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

namespace {

const char kClassName[] = "NodeQuicServer";
const char kOnSessionClosed[] = "onSessionClosed";
const char kOnSessionCreated[] = "onSessionCreated";
const char kOnStreamClosed[] = "onStreanClosed";
const char kOnStreamCreated[] = "onStreamCreated";

}  // namespace anonymous

NodeQuicServerWrap::NodeQuicServerWrap(const std::string& cert_path,
                                       const std::string& key_path,
                                       Local<Object> node_object)
    : quic_server_(new NodeQuicServer(cert_path, key_path, this)),
      isolate_(node_object->GetIsolate()),
      persist_object_(node_object->GetIsolate(), node_object) {
}

NodeQuicServerWrap::~NodeQuicServerWrap() {}

void NodeQuicServerWrap::OnStreamCreated(NodeQuicServerSession* session,
                                         NodeQuicServerStream* stream) {
  HandleScope scope(isolate_);
  Local<Context> context = isolate_->GetCurrentContext();
  Context::Scope context_scope(context);

  Local<Object> node_server = Local<Object>::New(isolate_, persist_object_);
  Local<Function> callback =
      Local<Function>::Cast(
          node_server->Get(
              context,
              Converter<std::string>::ToV8(isolate_, kOnStreamCreated))
          .ToLocalChecked());
  Local<Value> args[] = {
    Number::New(isolate_, session->connection_id()),
    NodeQuicServerStreamWrap::NewInstance(isolate_, stream),
  };
  callback->Call(node_server, arraysize(args), args);
}

void NodeQuicServerWrap::OnStreamClosed(NodeQuicServerSession* session,
                                        NodeQuicServerStream* stream) {
  HandleScope scope(isolate_);
  Local<Context> context = isolate_->GetCurrentContext();
  Context::Scope context_scope(context);

  Local<Object> node_server = Local<Object>::New(isolate_, persist_object_);
  Local<Function> callback =
      Local<Function>::Cast(
          node_server->Get(
              context,
              Converter<std::string>::ToV8(isolate_, kOnStreamClosed))
          .ToLocalChecked());
  Local<Value> args[] = {
    Number::New(isolate_, session->connection_id()),
    Number::New(isolate_, stream->id()),
  };
  callback->Call(node_server, arraysize(args), args);
}

void NodeQuicServerWrap::OnSessionCreated(NodeQuicServerSession* session) {
  HandleScope scope(isolate_);
  Local<Context> context = isolate_->GetCurrentContext();
  Context::Scope context_scope(context);

  Local<Object> node_server = Local<Object>::New(isolate_, persist_object_);
  Local<Function> callback =
      Local<Function>::Cast(
          node_server->Get(
              context,
              Converter<std::string>::ToV8(isolate_, kOnSessionCreated))
          .ToLocalChecked());
  Local<Value> args[] = {
    Number::New(isolate_, session->connection_id()),
  };
  callback->Call(node_server, arraysize(args), args);
}

void NodeQuicServerWrap::OnSessionClosed(NodeQuicServerSession* session,
                                         net::QuicErrorCode error,
                                         const std::string& error_details,
                                         net::ConnectionCloseSource source) {
  HandleScope scope(isolate_);
  Local<Context> context = isolate_->GetCurrentContext();
  Context::Scope context_scope(context);

  Local<Object> node_server = Local<Object>::New(isolate_, persist_object_);
  Local<Function> callback =
      Local<Function>::Cast(
          node_server->Get(
              context,
              Converter<std::string>::ToV8(isolate_, kOnSessionClosed))
          .ToLocalChecked());
  Local<Value> args[] = {
    Number::New(isolate_, session->connection_id()),
  };
  callback->Call(node_server, arraysize(args), args);
}

// static
void NodeQuicServerWrap::Init(Isolate* isolate) {
  Local<FunctionTemplate> tmpl = FunctionTemplate::New(isolate, &New);
  tmpl->SetClassName(String::NewFromUtf8(isolate, kClassName));
  tmpl->InstanceTemplate()->SetInternalFieldCount(1);

  NODE_SET_PROTOTYPE_METHOD(tmpl, "initialize", &Initialize);
  NODE_SET_PROTOTYPE_METHOD(tmpl, "shutdown", &Shutdown);
  NODE_SET_PROTOTYPE_METHOD(tmpl, "listen", &Listen);

  constructor_.Reset(isolate, tmpl->GetFunction());
}

// static
void NodeQuicServerWrap::NewInstance(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);

  if (!args[0]->IsString() || !args[1]->IsString() || !args[2]->IsObject()) {
    isolate->ThrowException(
        Exception::TypeError(
            String::NewFromUtf8(isolate, "invalid arguments")));
    return;
  }

  Local<Context> context = isolate->GetCurrentContext();
  Context::Scope context_scope(context);

  Local<Function> constructor = Local<Function>::New(isolate, constructor_);
  Local<Value> argv[] = { args[0], args[1], args[2] };
  Local<Object> instance =
      constructor->NewInstance(context, arraysize(argv), argv)
      .ToLocalChecked();

  args.GetReturnValue().Set(instance);
}

// static
void NodeQuicServerWrap::New(const FunctionCallbackInfo<Value>& args) {
  CHECK(args.IsConstructCall());
  CHECK_EQ(args.Length(), 3);

  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);

  std::string cert_path(*static_cast<String::Utf8Value>(args[0]->ToString()));
  std::string key_path(*static_cast<String::Utf8Value>(args[1]->ToString()));
  Local<Object> self = Local<Object>::Cast(args[2]);
  NodeQuicServerWrap* server_wrap =
      new NodeQuicServerWrap(cert_path, key_path, self);
  server_wrap->Wrap(args.This());

  args.GetReturnValue().Set(args.This());
}

// static
void NodeQuicServerWrap::Initialize(const FunctionCallbackInfo<Value>& args) {
  NodeQuicServerWrap* wrap =
      ObjectWrap::Unwrap<NodeQuicServerWrap>(args.Holder());
  NodeQuicServer* server = wrap->quic_server_.get();
  server->Initialize();
}

// static
void NodeQuicServerWrap::Shutdown(const FunctionCallbackInfo<Value>& args) {
  NodeQuicServerWrap* wrap =
      ObjectWrap::Unwrap<NodeQuicServerWrap>(args.Holder());
  NodeQuicServer* server = wrap->quic_server_.get();
  server->Shutdown();
}

// static
void NodeQuicServerWrap::Listen(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);

  if (!args[0]->IsString() || !args[1]->IsNumber()) {
    isolate->ThrowException(
        Exception::TypeError(
            String::NewFromUtf8(isolate, "invalid listen arguments")));
    return;
  }

  // listen ip
  String::Utf8Value ip(args[0]->ToString());
  net::IPAddress server_ip;
  if (!server_ip.AssignFromIPLiteral(std::string(*ip))) {
    isolate->ThrowException(
        Exception::TypeError(
            String::NewFromUtf8(isolate, "invalid listen ip argument")));
    return;
  }

  // listen port
  uint16_t server_port = static_cast<uint16_t>(args[1]->NumberValue());

  NodeQuicServerWrap* wrap =
      ObjectWrap::Unwrap<NodeQuicServerWrap>(args.Holder());

  net::IPEndPoint server_address(server_ip, server_port);
  NodeQuicServer* server = wrap->quic_server_.get();

  if (server->Listen(server_address) != net::OK) {
    isolate->ThrowException(
        Exception::TypeError(
            String::NewFromUtf8(isolate, "listen failed")));
    return;
  }
}

// static
Persistent<Function> NodeQuicServerWrap::constructor_;

}  // namespace stellite
