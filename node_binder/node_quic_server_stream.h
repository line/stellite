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

#ifndef NODE_BINDER_NODE_QUIC_SERVER_STREAM_H_
#define NODE_BINDER_NODE_QUIC_SERVER_STREAM_H_

#include "base/macros.h"
#include "net/quic/core/quic_spdy_stream.h"
#include "stellite/include/stellite_export.h"

namespace stellite {

class NodeQuicServerStream : public net::QuicSpdyStream {
 public:
  class Visitor {
   public:
    virtual ~Visitor() {}

    // called when headers are availble.
    virtual void OnHeadersAvailable(NodeQuicServerStream* stream,
                                    const net::SpdyHeaderBlock& headers,
                                    bool fin) = 0;

    // called when data are availble.
    virtual void OnDataAvailable(NodeQuicServerStream* stream,
                                 const char* buffer, size_t len, bool fin) = 0;
  };

  NodeQuicServerStream(net::QuicStreamId id, net::QuicSpdySession* session);
  ~NodeQuicServerStream() override;

  // implements QuicSpdyStream interface
  void OnInitialHeadersComplete(
      bool fin, size_t frame_len) override;

  void OnInitialHeadersComplete(
      bool fin, size_t frame_len,
      const net::QuicHeaderList& header_list) override;

  void OnDataAvailable() override;

  void set_visitor(Visitor* visitor) {
    visitor_ = visitor;
  }

 private:
  void NotifyDelegateOfHeadersComplete(net::SpdyHeaderBlock headers, bool fin);
  void NotifyDelegateOfDataAvailable(const char* buffer, size_t len, bool fin);
  void NotifyDelegateOfFinRead();

  net::SpdyHeaderBlock request_headers_;
  int64_t content_length_;

  Visitor* visitor_;

  DISALLOW_COPY_AND_ASSIGN(NodeQuicServerStream);
};

}  // namespace stellite

#endif  // NODE_BINDER_NODE_QUIC_SERVER_STREAM_H_
