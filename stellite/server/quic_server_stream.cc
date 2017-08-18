// Copyright 2016 LINE Corporation
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

#include "stellite/server/quic_server_stream.h"

#include "net/quic/core/spdy_utils.h"
#include "base/strings/string_number_conversions.h"

namespace net {

const char* kErrorResponseBody = "internal error";
const char* kHeaderStatus = ":status";
const char* kHeaderContentLength = ":content-length";

QuicServerStream::QuicServerStream(QuicStreamId id,
                                   QuicSpdySession* session)
    : QuicSpdyStream(id, session),
      content_length_(-1),
      content_received_(0) {
}

QuicServerStream:: ~QuicServerStream() {}

void QuicServerStream::CloseWriteSide() {
  if (!fin_received() && !rst_received() &&
      sequencer()->ignore_read_data() &&
      !rst_sent()) {
    // Early cancel the stream if it has stopped reading before receiving FIN
    // or RST.
    DCHECK(fin_sent());
    Reset(QUIC_STREAM_NO_ERROR);
  }

  QuicSpdyStream::CloseWriteSide();
}

void QuicServerStream::StopReading() {
  if (!fin_received() && !rst_received() && write_side_closed() &&
      !rst_sent()) {
    DCHECK(fin_sent());
    Reset(QUIC_STREAM_NO_ERROR);
  }

  QuicSpdyStream::StopReading();
}

void QuicServerStream::OnInitialHeadersComplete(bool fin, size_t frame_len) {
  QuicSpdyStream::OnInitialHeadersComplete(fin, frame_len);
  if (!SpdyUtils::ParseHeaders(decompressed_headers().data(),
                               decompressed_headers().length(),
                               &content_length_, &request_headers_)) {
    SendErrorResponse();
  }
  MarkHeadersConsumed(decompressed_headers().length());

  OnHeaderAvailable(fin);
}

void QuicServerStream::OnInitialHeadersComplete(
    bool fin,
    size_t frame_len,
    const QuicHeaderList& header_list) {
  QuicSpdyStream::OnInitialHeadersComplete(fin, frame_len, header_list);
  if (!SpdyUtils::CopyAndValidateHeaders(header_list, &content_length_,
                                         &request_headers_)) {
    SendErrorResponse();
  }
  ConsumeHeaderList();

  OnHeaderAvailable(fin);
}

void QuicServerStream::OnTrailingHeadersComplete(bool fin, size_t frame_len) {
  SendErrorResponse();
}

void QuicServerStream::OnTrailingHeadersComplete(
    bool fin,
    size_t frame_len,
    const QuicHeaderList& header_list) {
  SendErrorResponse();
}

void QuicServerStream::OnDataAvailable() {
  if (request_headers_.empty()) {
    SendErrorResponse();
    return;
  }

  while (HasBytesToRead()) {
    struct iovec iov;
    if (GetReadableRegions(&iov, 1) == 0) {
      break;
    }
    content_received_ += iov.iov_len;
    if (content_length_ >= 0 && content_received_ > content_length_) {
      SendErrorResponse();
      return;
    }

    std::string buffer(static_cast<char*>(iov.iov_base), iov.iov_len);
    MarkConsumed(iov.iov_len);

    OnContentAvailable(buffer.data(), buffer.size(), sequencer()->IsClosed());
  }

  if (!sequencer()->IsClosed()) {
    sequencer()->SetUnblocked();
    return;
  }

  OnFinRead();
}

void QuicServerStream::SendErrorResponse() {
  SendErrorResponse(500, "internal error");
}

void QuicServerStream::SendErrorResponse(int status_code,
                                         const std::string& message) {
  SpdyHeaderBlock headers;
  headers[kHeaderStatus] = base::UintToString(status_code);
  headers[kHeaderContentLength] = base::UintToString(message.size());
  SendHeadersAndBody(std::move(headers), message);
}

void QuicServerStream::SendHeadersAndBody(SpdyHeaderBlock response_headers,
                                          base::StringPiece body) {
  SendHeadersAndBodyAndTrailers(std::move(response_headers), body,
                                SpdyHeaderBlock());
}

void QuicServerStream::SendHeadersAndBodyAndTrailers(
    SpdyHeaderBlock response_headers,
    base::StringPiece body,
    SpdyHeaderBlock response_trailers) {
  if (!reading_stopped()) {
    StopReading();
  }

  bool send_fin = (body.empty() && response_trailers.empty());
  WriteHeaders(std::move(response_headers), send_fin, nullptr);
  if (send_fin) {
    return;
  }

  send_fin = response_trailers.empty();
  if (!body.empty() || send_fin) {
    WriteOrBufferData(body, send_fin, nullptr);
  }

  if (send_fin) {
    // Nothing else to send.
    return;
  }

  WriteTrailers(std::move(response_trailers), nullptr);
}

void QuicServerStream::OnHeaderAvailable(bool fin) {}
void QuicServerStream::OnContentAvailable(const char* data, size_t len,
                                          bool fin) {}

}  // namespace net
