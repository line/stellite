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

#ifndef STELLITE_SERVER_QUIC_SERVER_STREAM_H_
#define STELLITE_SERVER_QUIC_SERVER_STREAM_H_

#include "net/quic/core/quic_spdy_stream.h"

namespace net {

class QuicServerStream : public QuicSpdyStream {
 public:
  QuicServerStream(QuicStreamId id, QuicSpdySession* session);
  ~QuicServerStream() override;

  // override QuicStream
  void CloseWriteSide() override;
  void StopReading() override;

  // QuicSpdyStream
  void OnInitialHeadersComplete(bool fin, size_t frame_len) override;
  void OnInitialHeadersComplete(bool fin,
                                size_t frame_len,
                                const QuicHeaderList& header_list) override;

  // QuicSpdyStream
  void OnTrailingHeadersComplete(bool fin, size_t frame_len) override;
  void OnTrailingHeadersComplete(bool fin,
                                 size_t frame_len,
                                 const QuicHeaderList& header_list) override;

  void OnDataAvailable() override;

  virtual void OnHeaderAvailable(bool fin);
  virtual void OnContentAvailable(const char* data, size_t len, bool fin);

  SpdyHeaderBlock* request_headers() { return &request_headers_; }
  int64_t content_length() { return content_length_; }
  int64_t content_received() { return content_received_; }

 protected:
  virtual void SendErrorResponse();
  virtual void SendErrorResponse(int status_code, const std::string& message);

  virtual void SendHeadersAndBody(SpdyHeaderBlock response_headers,
                                  base::StringPiece body);
  virtual void SendHeadersAndBodyAndTrailers(SpdyHeaderBlock response_headers,
                                             base::StringPiece body,
                                             SpdyHeaderBlock response_trailers);

 private:
  SpdyHeaderBlock request_headers_;
  int64_t content_length_;
  int64_t content_received_;

  DISALLOW_COPY_AND_ASSIGN(QuicServerStream);
};

}  // namespace net
#endif  // STELLITE_SERVER_QUIC_SERVER_STREAM_BASE_H_
