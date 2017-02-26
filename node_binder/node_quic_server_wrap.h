//
// 2017 write by snibug@gmail.com
//

#ifndef NODE_BINDER_NODE_QUIC_SERVER_WRAP_H_
#define NODE_BINDER_NODE_QUIC_SERVER_WRAP_H_

#include <memory>

#include "base/macros.h"
#include "node/node.h"
#include "node/node_object_wrap.h"
#include "node_binder/node_quic_notify_interface.h"

namespace stellite {

class NodeQuicServer;

class NodeQuicServerWrap : public node::ObjectWrap,
                           public NodeQuicNotifyInterface {
 public:
  static void Init(v8::Isolate* isolate);
  static void NewInstance(const v8::FunctionCallbackInfo<v8::Value>& args);

  // implements NodeQuicNotifyInterface
  void OnStreamCreated(NodeQuicServerSession* session,
                       NodeQuicServerStream* stream) override;
  void OnStreamClosed(NodeQuicServerSession* session,
                      NodeQuicServerStream* stream) override;
  void OnSessionCreated(NodeQuicServerSession* session) override;
  void OnSessionClosed(NodeQuicServerSession* session,
                       net::QuicErrorCode error,
                       const std::string& error_details,
                       net::ConnectionCloseSource source) override;

  NodeQuicServer* quic_server() {
    return quic_server_.get();
  }

 private:
  NodeQuicServerWrap(const std::string& cert_path, const std::string& key_path,
                     v8::Local<v8::Object> node_object);
  ~NodeQuicServerWrap() override;

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Initialize(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Shutdown(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Listen(const v8::FunctionCallbackInfo<v8::Value>& args);

  static v8::Persistent<v8::Function> constructor_;

  std::unique_ptr<NodeQuicServer> quic_server_;

  v8::Isolate* isolate_;
  v8::Persistent<v8::Object> persist_object_;

  DISALLOW_COPY_AND_ASSIGN(NodeQuicServerWrap);
};

}  // namespace stellite

#endif  // NODE_BINDER_NODE_QUIC_SERVER_WRAP_H_
