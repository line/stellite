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

#ifndef STELLITE_SERVER_QUIC_PROXY_STREAM_H_
#define STELLITE_SERVER_QUIC_PROXY_STREAM_H_

#include "base/memory/weak_ptr.h"
#include "stellite/server/quic_server_stream.h"
#include "stellite/fetcher/http_fetcher_task.h"

namespace stellite {
class HttpFetcher;
}  // namespace stellite

namespace net {

class QuicProxyStream : public QuicServerStream,
                        public stellite::HttpFetcherTask::Visitor {
 public:
  QuicProxyStream(QuicStreamId id, QuicSpdySession* session,
                  stellite::HttpFetcher* http_fetcher,
                  GURL proxy_pass);
  ~QuicProxyStream() override;

  void SendRequest(const std::string& body);
  void AppendChunkToUpload(const char* data, size_t len, bool fin);

  // implements QuicServerStream: quic client -> quic server
  void OnHeaderAvailable(bool fin) override;
  void OnContentAvailable(const char* data, size_t len, bool fin) override;

  // implements HttpFetcherTask::Visitor: backend -> quic server
  void OnTaskComplete(int request_id,
                      const URLFetcher* source,
                      const HttpResponseInfo* response_info) override;
  void OnTaskHeader(int request_id,
                    const URLFetcher* source,
                    const HttpResponseInfo* response_info) override;
  void OnTaskStream(int request_id,
                    const char* data, size_t len, bool fin) override;
  void OnTaskError(int request_id,
                   const URLFetcher* source,
                   int error_code) override;

 private:
  GURL GetProxyRequestURL(const SpdyHeaderBlock& headers);

  bool is_chunked_upload_;

  int backend_request_id_;
  GURL proxy_pass_;
  std::stringstream payload_stream_;

  stellite::HttpFetcher* http_fetcher_;

  base::WeakPtrFactory<QuicProxyStream> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(QuicProxyStream);
};

}  // namespace net

#endif  // STELLITE_SERVER_QUIC_PROXY_STREAM_H_
