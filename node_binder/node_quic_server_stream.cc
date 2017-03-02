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

#include "node_binder/node_quic_server_stream.h"

#include "base/logging.h"
#include "net/quic/core/quic_spdy_session.h"
#include "net/quic/core/spdy_utils.h"
#include "net/spdy/spdy_framer.h"

#include "base/strings/string_number_conversions.h"

namespace stellite {

NodeQuicServerStream::NodeQuicServerStream(net::QuicStreamId id,
                                           net::QuicSpdySession* session)
    : net::QuicSpdyStream(id, session),
      content_length_(0),
      visitor_(nullptr) {
}

NodeQuicServerStream::~NodeQuicServerStream() {}

void NodeQuicServerStream::OnInitialHeadersComplete(bool fin,
                                                    size_t frame_len) {
  QuicSpdyStream::OnInitialHeadersComplete(fin, frame_len);
  if (!net::SpdyUtils::ParseHeaders(decompressed_headers().data(),
                                    decompressed_headers().length(),
                                    &content_length_, &request_headers_)) {
    DVLOG(1) << "Invalid headers";
  }
  MarkHeadersConsumed(decompressed_headers().length());
  NotifyDelegateOfHeadersComplete(std::move(request_headers_), fin);
}

void NodeQuicServerStream::OnInitialHeadersComplete(
    bool fin, size_t frame_len,
    const net::QuicHeaderList& header_list) {
  net::QuicSpdyStream::OnInitialHeadersComplete(fin, frame_len, header_list);
  if (!net::SpdyUtils::CopyAndValidateHeaders(header_list, &content_length_,
                                              &request_headers_)) {
    LOG(ERROR) << "failed to parse header list: " << header_list.DebugString();
    ConsumeHeaderList();
    Reset(net::QUIC_BAD_APPLICATION_PAYLOAD);
    return;
  }
  ConsumeHeaderList();
  NotifyDelegateOfHeadersComplete(std::move(request_headers_), fin);
}

void NodeQuicServerStream::OnDataAvailable() {
  while (HasBytesToRead()) {
    struct iovec iov;
    if (GetReadableRegions(&iov, 1) == 0) {
      break;
    }

    std::string data(static_cast<char*>(iov.iov_base), iov.iov_len);
    MarkConsumed(iov.iov_len);

    // notify data received
    NotifyDelegateOfDataAvailable(data.data(), data.size(),
                                  sequencer()->IsClosed());
  }

  if (!sequencer()->IsClosed()) {
    sequencer()->SetUnblocked();
    return;
  }

  OnFinRead();

  if (write_side_closed() || fin_buffered()) {
    return;
  }
}

void NodeQuicServerStream::NotifyDelegateOfHeadersComplete(
    net::SpdyHeaderBlock headers, bool fin) {
  if (visitor_) {
    visitor_->OnHeadersAvailable(this, headers, fin);
  }
}

void NodeQuicServerStream::NotifyDelegateOfDataAvailable(
    const char* buffer, size_t len, bool fin) {
  if (visitor_) {
    visitor_->OnDataAvailable(this, buffer, len, fin);
  }
}

}  // namespace stellite
