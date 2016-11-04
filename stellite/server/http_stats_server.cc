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

#include "stellite/server/http_stats_server.h"

#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/log/net_log.h"
#include "net/socket/tcp_server_socket.h"
#include "stellite/logging/logging.h"

namespace {

const char kUriPathMain[] = "/";
const char kUriPathStatsHttp[] = "/stats/http";
const char kUriPathStatsQuic[] = "/stats/quic";
const char kUriPathStatsNow[] = "/stats/now";

const char kMainPageHtml[] =
    "<html><body><p>quic server is running</p><ul>\n"
    "<li><a href='/stats/http'>/stat/http</a></li>\n"
    "<li><a href='/stats/quic'>/stat/quic</a></li>\n"
    "<li><a href='/stats/resource'>/stat/resource</a></li>\n"
    "<li><a href='/stats/now'>/stat/now</a></li>\n"
    "</ul></body></html>\n";

} // namespace anonymous

namespace net {

bool IsValidPath(const std::string& request_path) {
  return request_path == kUriPathMain ||
         request_path == kUriPathStatsHttp ||
         request_path == kUriPathStatsQuic ||
         request_path == kUriPathStatsNow;
}

HttpStatsServer::HttpStatsServer(Delegate* stats_delegate)
    : stats_delegate_(stats_delegate) {
}

HttpStatsServer::~HttpStatsServer() {
  Shutdown();
}

bool HttpStatsServer::Start(uint16_t port) {
  if (http_server_.get()) {
    return true;
  }

  std::unique_ptr<ServerSocket> server_socket(
      new TCPServerSocket(nullptr, NetLog::Source()));

  std::string listen_address = "0.0.0.0";
  server_socket->ListenWithAddressAndPort(listen_address, port, 1);
  http_server_.reset(new HttpServer(std::move(server_socket), this));

  IPEndPoint address;
  if (http_server_->GetLocalAddress(&address) != OK) {
    FLOG(ERROR) << "Cannot start HTTP server.";
    return false;
  }

  return true;
}

void HttpStatsServer::Shutdown() {
  if (!http_server_.get()) {
    return;
  }

  http_server_.reset(nullptr);
}

void HttpStatsServer::OnConnect(int connection_id) {
}

void HttpStatsServer::OnClose(int connection_id) {
}

void HttpStatsServer::OnWebSocketRequest(int connection_id,
                                         const HttpServerRequestInfo& info) {
}

void HttpStatsServer::OnWebSocketMessage(int connection_id,
                                         const std::string& data) {
}

void HttpStatsServer::OnHttpRequest(int connection_id,
                                    const HttpServerRequestInfo& info) {
 GURL url("http://host" + info.path);

  if (!ValidateRequestMethod(connection_id, url.path(), info.method)) {
    FLOG(ERROR) << "Unknown path request: " << url.path();
    return;
  }

  std::string response;
  std::string mime_type;
  HttpStatusCode status_code = ProcessHttpRequest(url, info, &response,
                                                  &mime_type);
  http_server_->Send(connection_id, status_code, response, mime_type);
}

HttpStatusCode HttpStatsServer::ProcessHttpRequest(
    const GURL& url,
    const HttpServerRequestInfo& info,
    std::string* response,
    std::string* mime_type) {

  DCHECK(response);
  DCHECK(mime_type);

  if (url.path() == kUriPathMain) {
    return ProcessMain(response, mime_type);
  } else if (url.path() == kUriPathStatsHttp) {
    return ProcessHttpStats(response, mime_type);
  } else if (url.path() == kUriPathStatsQuic) {
    return ProcessQuicStats(response, mime_type);
  } else if (url.path() == kUriPathStatsNow) {
    return ProcessNowStats(response, mime_type);
  }

  return HTTP_NOT_FOUND;
}

HttpStatusCode HttpStatsServer::ProcessMain(std::string* response,
                                            std::string* mime_type) {
  mime_type->clear();
  mime_type->append("text/html");
  response->append(kMainPageHtml);
  return HTTP_OK;
}

HttpStatusCode HttpStatsServer::ProcessHttpStats(std::string* response,
                                                 std::string* mime_type) {
  const HttpStats& http_stats = stats_delegate_->http_stats();

  std::unique_ptr<base::DictionaryValue> root(new base::DictionaryValue());

  root->SetString("http_sent",
                  base::Uint64ToString(http_stats.http_sent));
  root->SetString("http_timeout",
                  base::Uint64ToString(http_stats.http_timeout));
  root->SetString("http_connect_failed",
                  base::Uint64ToString(http_stats.http_connection_failed));
  root->SetString("http_received",
                  base::Uint64ToString(http_stats.http_received));

  response->clear();
  base::JSONWriter::WriteWithOptions(*root.get(),
                                     base::JSONWriter::OPTIONS_PRETTY_PRINT,
                                     response);

  mime_type->clear();
  mime_type->append("application/json");

  return HTTP_OK;
}

HttpStatusCode HttpStatsServer::ProcessQuicStats(std::string* response,
                                                 std::string* mime_type) {
  const QuicStats& quic_stats = stats_delegate_->quic_stats();
  std::unique_ptr<base::DictionaryValue> root(new base::DictionaryValue());

  root->SetString("packets_sent",
                  base::Uint64ToString(quic_stats.packets_sent));
  root->SetString("packets_received",
                  base::Uint64ToString(quic_stats.packets_received));
  root->SetString("packets_lost",
                  base::Uint64ToString(quic_stats.packets_lost));
  root->SetString("packets_discarded",
                  base::Uint64ToString(quic_stats.packets_discarded));
  root->SetString("packets_processed",
                  base::Uint64ToString(quic_stats.packets_processed));
  root->SetString("packets_revived",
                  base::Uint64ToString(quic_stats.packets_revived));
  root->SetString("packets_dropped",
                  base::Uint64ToString(quic_stats.packets_dropped));
  root->SetString("packets_reordered",
                  base::Uint64ToString(quic_stats.packets_dropped));
  root->SetString("slowstart_packets_sent",
                  base::Uint64ToString(quic_stats.slowstart_packets_sent));
  root->SetString("slowstart_packets_lost",
                  base::Uint64ToString(quic_stats.slowstart_packets_lost));

  response->clear();
  base::JSONWriter::WriteWithOptions(*root.get(),
                                     base::JSONWriter::OPTIONS_PRETTY_PRINT,
                                     response);

  mime_type->clear();
  mime_type->append("application/json");

  return HTTP_OK;
}

HttpStatusCode HttpStatsServer::ProcessNowStats(std::string* response,
                                                std::string* mime_type) {
  return HTTP_INTERNAL_SERVER_ERROR;

  /*
  response->clear();
  mime_type->clear();

  const double diff = receiver->diff().InSecondsF();
  if (diff <= 0.f) {
    mime_type->append("text/html");
    response->append("invalid quic server interval time.");
    return HTTP_INTERNAL_SERVER_ERROR;
  }

  // HTTP TPS
  base::DictionaryValue* http_node = new base::DictionaryValue();
  const HttpStats& http_diff_stats = receiver->diff_http_stats();

  http_node->SetDouble("http_sent_per_sec",
                       http_diff_stats.http_sent / diff);
  http_node->SetDouble("http_timeout_per_sec",
                       http_diff_stats.http_timeout / diff);
  http_node->SetDouble("http_connection_failed_per_sec",
                       http_diff_stats.http_connection_failed / diff);
  http_node->SetDouble("http_received_per_sec",
                       http_diff_stats.http_received / diff);

  // QUIC TPS
  base::DictionaryValue* quic_node = new base::DictionaryValue();
  const QuicDispatcherStats& quic_stats = receiver->diff_quic_stats();

  quic_node->SetDouble("quic_packet_sent_per_sec",
                       quic_stats.packets_sent / diff);
  quic_node->SetDouble("quic_packets_received_per_sec",
                       quic_stats.packets_received / diff);
  quic_node->SetDouble("quic_packets_lost_per_sec",
                       quic_stats.packets_lost / diff);
  quic_node->SetDouble("quic_packets_retransmitted_per_sec",
                       quic_stats.packets_retransmitted / diff);
  quic_node->SetDouble("quic_packets_discarded_per_sec",
                       quic_stats.packets_discarded / diff);
  quic_node->SetDouble("quic_packets_processed_per_sec",
                       quic_stats.packets_processed / diff);
  quic_node->SetDouble("quic_packets_revived_per_sec",
                       quic_stats.packets_revived / diff);
  quic_node->SetDouble("quic_packets_dropped_per_sec",
                       quic_stats.packets_dropped / diff);
  quic_node->SetDouble("quic_packets_reordered_per_sec",
                       quic_stats.packets_reordered / diff);
  quic_node->SetDouble("quic_packets_slowstart_packets_sent_per_sec",
                       quic_stats.slowstart_packets_sent / diff);
  quic_node->SetDouble("quic_packets_slowstart_packets_lost_per_sec",
                       quic_stats.slowstart_packets_lost / diff);

  // Active stats
  base::DictionaryValue* active_node = new base::DictionaryValue();

  // Active HTTP request count
  HttpStats http_stats;

  const HttpStatsMap& http_stats_map = receiver->http_stats();
  HttpStatsMap::const_iterator http_stats_it = http_stats_map.begin();
  while (http_stats_it != http_stats_map.end()) {
    http_stats.Add(http_stats_it->second);
    ++http_stats_it;
  }

  QuicStatCount http_active_request = http_stats.http_sent;
  http_active_request -= http_stats.http_received;
  http_active_request -= http_stats.http_timeout;
  http_active_request -= http_stats.http_connection_failed;
  active_node->SetInteger("http_active_request", http_active_request);

  // Active QUIC connection count
  QuicActiveStats active_stats;
  const QuicActiveStatsMap& active_stats_map = receiver->active_stats();

  QuicActiveStatsMap::const_iterator active_stats_it = active_stats_map.begin();
  while (active_stats_it != active_stats_map.end()) {
    active_stats.Add(active_stats_it->second);
    ++active_stats_it;
  }
  active_node->SetInteger("quic_active_connection",
                          active_stats.quic_active_connection);

  // Server stats summary
  base::DictionaryValue* server_node = new base::DictionaryValue();

  // CPU summary
  double cpu_usage = 0.0;
  int64 private_memory = 0;
  int64 shared_memory = 0;
  const QuicProcessMetricsMap& process_metrics_map = service->process_metrics();

  QuicProcessMetricsMap::const_iterator it = process_metrics_map.begin();
  while (it != process_metrics_map.end()) {
    base::ProcessMetrics* metrics = it->second;
    cpu_usage += metrics->GetCPUUsage();

    size_t private_bytes, shared_bytes;
    if (metrics->GetMemoryBytes(&private_bytes, &shared_bytes)) {
      private_memory += private_bytes;
      shared_memory += shared_bytes;
    }
    ++it;
  }
  server_node->SetDouble("cpu", cpu_usage);

  int64 memory = shared_memory + private_memory;
  int32 memory_mb = static_cast<int32>(memory / 1048576);
  server_node->SetInteger("memory_mb", memory_mb);

  // Summary
  std::unique_ptr<base::DictionaryValue> root(new base::DictionaryValue());
  root->Set("quic_stats", quic_node);
  root->Set("http_stats", http_node);
  root->Set("active_stats", active_node);
  root->Set("server_stats", server_node);

  base::JSONWriter::WriteWithOptions(*root.get(),
                                     base::JSONWriter::OPTIONS_PRETTY_PRINT,
                                     response);

  mime_type->append("application/json");
  return HTTP_OK;
  */
}

bool HttpStatsServer::ValidateRequestMethod(int connection_id,
                                            const std::string& request,
                                            const std::string& method) {
  if (!IsValidPath(request)) {
    http_server_->Send404(connection_id);
    return false;
  }

  if (method != "GET") {
    http_server_->Send404(connection_id);
    return false;
  }

  return true;
}

} // namespace net
