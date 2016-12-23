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

#ifndef QUIC_SERVER_QUIC_SERVER_CONFIG_H_
#define QUIC_SERVER_QUIC_SERVER_CONFIG_H_

#include "base/files/file_path.h"
#include "base/values.h"
#include "stellite/fetcher/http_rewrite.h"
#include "url/gurl.h"
#include "net/base/net_export.h"

namespace base {
class FilePath;
class CommandLine;
}

namespace net {

class NET_EXPORT ServerConfig {
 public:
  ServerConfig();
  ~ServerConfig();

  void PrintHelpMessages();

  // Parse command line
  bool ParseCommandLine(const base::CommandLine* command_line);

  // Parse JSON configure file
  bool ParseConfigFile(const base::FilePath& file_path);

  bool daemon() {
    return daemon_;
  }

  bool stop() {
    return stop_;
  }

  bool logging() {
    return logging_;
  }

  bool file_logging() {
    return file_logging_;
  }

  uint32_t worker_count() const {
    return static_cast<uint32_t>(worker_count_);
  }

  uint32_t dispatch_continuity() const {
    return static_cast<uint32_t>(dispatch_continuity_);
  }

  uint32_t send_buffer_size() const {
    return static_cast<uint32_t>(send_buffer_size_);
  }

  uint32_t recv_buffer_size() const {
    return static_cast<uint32_t>(recv_buffer_size_);
  }

  uint16_t quic_port() const {
    return static_cast<uint16_t>(quic_port_);
  }

  void quic_port(int port) {
    quic_port_ = port;
  }

  uint32_t proxy_timeout() const {
    return static_cast<uint32_t>(proxy_timeout_);
  }

  void proxy_timeout(const uint32_t timeout_seconds) {
    proxy_timeout_ = timeout_seconds;
  }

  // proxy_pass function filters URLs to use for original proxy URL
  GURL proxy_pass() const {
    return proxy_pass_;
  }

  void set_bind_address(const std::string bind_address) {
    bind_address_ = bind_address;
  }

  const std::string& bind_address() const {
    return bind_address_;
  }

  bool proxy_pass(const std::string& proxy_url) {
    proxy_pass_ = GURL(proxy_url).GetOrigin();
    return proxy_pass_.is_valid();
  }

  const RewriteRules& rewrite_rules() const {
    return rewrite_rules_;
  }

  void keyfile(const std::string& keyfile) {
    keyfile_ = base::FilePath(keyfile);
  }

  void keyfile(const base::FilePath& keyfile) {
    keyfile_ = keyfile;
  }

  const base::FilePath& keyfile() const {
    return keyfile_;
  }

  void certfile(const std::string& certfile) {
    certfile_ = base::FilePath(certfile);
  }

  void certfile(const base::FilePath& certfile) {
    certfile_ = certfile;
  }

  const base::FilePath& certfile() const {
    return certfile_;
  }

  void log_dir(const base::FilePath& log_dir) {
    log_dir_ = log_dir;
  }

  const base::FilePath& log_dir() {
    return log_dir_;
  }

 private:
  // Rewrite rule
  bool AddRewriteRule(const std::string& pattern,
                      const std::string& replace_format);

  bool daemon_;
  bool stop_;
  bool logging_;
  bool file_logging_;

  int worker_count_;
  int dispatch_continuity_;

  int send_buffer_size_;
  int recv_buffer_size_;

  int proxy_timeout_;

  uint16_t quic_port_;

  GURL proxy_pass_;
  std::string bind_address_;

  base::FilePath keyfile_;
  base::FilePath certfile_;

  RewriteRules rewrite_rules_;

  base::FilePath log_dir_;

  DISALLOW_COPY_AND_ASSIGN(ServerConfig);
};

} // namespace net

#endif // QUIC_SERVER_QUIC_SERVER_CONFIG_H_
