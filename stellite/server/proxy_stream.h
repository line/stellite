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

#ifndef STELLITE_SERVER_PROXY_STREAM_H_
#define STELLITE_SERVER_PROXY_STREAM_H_

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "stellite/fetcher/http_fetcher.h"
#include "stellite/server/server_config.h"
#include "net/tools/quic/quic_simple_server_stream.h"
#include "url/gurl.h"

namespace stellite {
class HttpFetcher;
}

namespace net {
class HttpResponseHeaders;
class HttpRewrite;
class ServerConfig;
class ServerSession;
class URLFetcher;

// ProxyStream is responsible for receiving QUIC HTTP requests, fetching
// backend requests, and responding with QUIC HTTP responses
class NET_EXPORT ProxyStream : public QuicSimpleServerStream,
                               public stellite::HttpFetcherTask::Visitor {
 public:
  ProxyStream(const ServerConfig& server_config,
              QuicStreamId id, ServerSession* session,
              stellite::HttpFetcher* http_fetcher,
              const HttpRewrite* http_rewrite);

  ~ProxyStream() override;

  // Implements HttpFetcherDelegate
  // OnFetchComplete may have success and error except timeout
  void OnTaskComplete(int request_id, const URLFetcher* source,
                      const HttpResponseInfo* response_info) override;

  void OnTaskHeader(int request_id, const URLFetcher* source,
                    const HttpResponseInfo* response_info) override;

  void OnTaskStream(int request_id, const char* data, size_t len,
                    bool fin) override;

  void OnTaskError(int request_id, const URLFetcher* source,
                   int error_code) override;

 protected:
  void SendResponse() override;

  void ErrorResponse(int error_code, const std::string& error_message,
                     int64_t msec);

  scoped_refptr<HttpResponseHeaders> BuildCustomHeader(
      const int status_code,
      const GURL& url,
      const std::string& contents);

  void WriteAccessLog(int response_code, int64_t msec);

 private:
  ServerSession* per_connection_session_;

  stellite::HttpFetcher* http_fetcher_; // Not owned

  const HttpRewrite* http_rewrite_;

  uint32_t proxy_timeout_;

  const GURL proxy_pass_;

  GURL proxy_url_;

  URLFetcher::RequestType proxy_method_;

  // HTTP fetcher may have accessed after a proxy stream was released
  base::WeakPtrFactory<ProxyStream> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ProxyStream);
};

} // namespace net

#endif // STELLITE_THREAD_PROXY_STREAM_H_
