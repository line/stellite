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

#ifndef STELLITE_PROCESS_HTTP_STATS_SERVER_H_
#define STELLITE_PROCESS_HTTP_STATS_SERVER_H_

#include <memory>

#include "net/base/net_export.h"
#include "net/server/http_server.h"
#include "net/server/http_server_request_info.h"
#include "url/gurl.h"

namespace net {

// HttpStatsServer serves QUIC server status using the HTTP protocol
class NET_EXPORT HttpStatsServer: public HttpServer::Delegate {
 public:
  class Delegate {
   public:
    virtual ~Delegate() {}

    virtual const QuicStats& quic_stats() = 0;
    virtual const HttpStats& http_stats() = 0;
  };

  explicit HttpStatsServer(Delegate* stats_delegate);
  ~HttpStatsServer() override;

  bool Start(uint16_t port);
  void Shutdown();

  // Implements HttpServerDelegate methods
  void OnConnect(int connection_id) override;
  void OnClose(int connection_id) override;
  void OnWebSocketRequest(int connection_id,
                          const HttpServerRequestInfo& info) override;
  void OnWebSocketMessage(int connection_id,
                          const std::string& data) override;
  void OnHttpRequest(int connection_id,
                     const HttpServerRequestInfo& info) override;

 private:
  bool ValidateRequestMethod(int connection_id,
                             const std::string& request,
                             const std::string& method);

  HttpStatusCode ProcessHttpRequest(
      const GURL& url,
      const HttpServerRequestInfo& info,
      std::string* response,
      std::string* mime_type);

  HttpStatusCode ProcessMain(std::string* response,
                             std::string* mime_type);

  HttpStatusCode ProcessHttpStats(std::string* response,
                                  std::string* mime_type);

  HttpStatusCode ProcessQuicStats(std::string* response,
                                  std::string* mime_type);

  HttpStatusCode ProcessNowStats(std::string* response,
                                 std::string* mime_type);

  Delegate* stats_delegate_; // not owned

  std::unique_ptr<HttpServer> http_server_;

  DISALLOW_COPY_AND_ASSIGN(HttpStatsServer);
};

} // namespace net

#endif // STELLITE_HTTP_QUIC_HTTP_SERVER_H_
