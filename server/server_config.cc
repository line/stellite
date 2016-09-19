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

#include "stellite/server/server_config.h"

#include <limits>
#include <memory>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "stellite/logging/logging.h"
#include "stellite/server/parse_util.h"
#include "url/gurl.h"

namespace net {
const int kDefaultDispatchContinuity = 16;
const int kDefaultHttpRequestTimeout = 30;
const int kDefaultQuicPort = 6121;
const int kDefaultRecvBufferSize = 256 * 1024;
const int kQuicMaxPacketSize = 1452;
const int kDefaultSendBufferSize = kQuicMaxPacketSize * 30;
const int kUpperBoundPort =
    static_cast<int>(std::numeric_limits<uint16_t>::max());

const char* kBindAddress = "bind_address";
const char* kCertfile = "certfile";
const char* kConfig = "config";
const char* kDaemon = "daemon";
const char* kDefaultBindAddress = "::";
const char* kDispatchContinuity = "dispatch_continuity";
const char* kKeyfile = "keyfile";
const char* kLogDir = "log_dir";
const char* kLogging = "logging";
const char* kProxyPass = "proxy_pass";
const char* kProxyTimeout = "proxy_timeout";
const char* kQuicPort = "quic_port";
const char* kRecvBufferSize = "recv_buffer_size";
const char* kRewrite = "rewrite";
const char* kSendBufferSize = "send_buffer_size";
const char* kStop = "stop";
const char* kWorkerCount = "worker_count";

ServerConfig::ServerConfig()
  : daemon_(false),
    stop_(false),
    logging_(false),
    worker_count_(1),
    dispatch_continuity_(kDefaultDispatchContinuity),
    send_buffer_size_(kDefaultSendBufferSize),
    recv_buffer_size_(kDefaultRecvBufferSize),
    proxy_timeout_(kDefaultHttpRequestTimeout),
    quic_port_(kDefaultQuicPort),
    proxy_pass_(),
    bind_address_(kDefaultBindAddress),
    rewrite_rules_() {
}

ServerConfig::~ServerConfig() {}

void ServerConfig::PrintHelpMessages() {
  const char* help_message =
    "Usage: quic_thread_server [options]\n"
    "\n"
    "options:\n"
    "-h, --help                     Show this help message and exit\n"
    "--quic_port=<port>             Specify the QUIC port number to listen on\n"
    "--proxy_timeout=<second>       Specify timeout duration to avoid a proxy\n"
    "                               response\n"
    "--worker_count=<count>         Specify the worker thread count\n"
    "--dispatch_continuity=<count>  Specify the dispatch continuity count\n"
    "                               range [1, 32], default is 16\n"
    "--send_buffer_size=<size>      Specify the send buffer size\n"
    "                               default size was 1452 * 30\n"
    "--recv_buffer_size=<size>      Specify the recv buffer size\n"
    "                               default size was 256 KB\n"
    "--daemon                       Daemonize a process\n"
    "--stop                         Stop a QUIC daemon process\n"
    "--proxy_pass=<url>             Reverse proxy URL\n"
    "                               Example: http://example.com:8080\n"
    "--config=<config_file_path>    Specify the QUIC server config file path\n"
    "--keyfile=<key_file_path>      Specify the SSL key file path\n"
    "--certfile=<cert_file_path>    Specify the SSL certificate file path\n"
    "--bind_address=<ip>            Specify IP address to bind UDP socket\n"
    "--log_dir=<log_dir>            Specify the logging directory\n"
    "--logging                      Turn on file-based logging\n"
    "                               (It is turned off by default)\n";
  LOG(ERROR) << help_message;
}

bool ServerConfig::ParseConfigFile(const base::FilePath& config_path) {
  base::FilePath abs_config_path = base::MakeAbsoluteFilePath(config_path);
  LOG(INFO) << "Parse config: " << abs_config_path.value();

  std::unique_ptr<base::DictionaryValue> server_config =
      base::DictionaryValue::From(ParseFromJsonFile(abs_config_path));
  if (!server_config->GetBoolean(kDaemon, &daemon_)) {
    LOG(ERROR) << "Server config: daemon option is not set";
    return false;
  }

  if (!server_config->GetInteger(kWorkerCount, &worker_count_)) {
    LOG(ERROR) << "Server config: worker_count option is not set";
    return false;
  }

  if (!server_config->GetInteger(kDispatchContinuity, &dispatch_continuity_)) {
    LOG(ERROR) << "Server config: dispatch_continuity is not set";
    return false;
  }

  if (!server_config->GetInteger(kSendBufferSize, &send_buffer_size_)) {
    LOG(ERROR) << "Server config: send_buffer_size is not set";
    return false;
  }

  if (!server_config->GetInteger(kRecvBufferSize, &recv_buffer_size_)) {
    LOG(ERROR) << "Server config: recv_buffer_size is not set";
    return false;
  }

  int quic_port;
  if (!server_config->GetInteger(kQuicPort, &quic_port)) {
    LOG(ERROR) << "Server config: quic_port option is not set";
    return false;
  }
  quic_port_ = static_cast<uint16_t>(quic_port);

  std::string proxy_pass_url;
  if (!server_config->GetString(kProxyPass, &proxy_pass_url)) {
    LOG(ERROR) << "Server config: proxy_pass option is not set";
    return false;
  }

  if (!proxy_pass(proxy_pass_url)) {
    LOG(ERROR) << "proxy_pass: proxy_pass URL scheme is invalid";
    return false;
  }

  if (!server_config->GetString(kBindAddress, &bind_address_)) {
    LOG(ERROR) << "Server config: bind_address option is not set";
    return false;
  }

  if (!server_config->GetInteger(kProxyTimeout, &proxy_timeout_)) {
    proxy_timeout_ = kDefaultHttpRequestTimeout;
  }

  std::string keyfile;
  if (!server_config->GetString(kKeyfile, &keyfile)) {
    LOG(ERROR) << "Server config: keyfile option is not set";
    return false;
  }
  keyfile_ = base::FilePath(keyfile);

  std::string certfile;
  if (!server_config->GetString(kCertfile, &certfile)) {
    LOG(ERROR) << "Server config: certfile option is not set";
    return false;
  }
  certfile_ = base::FilePath(certfile);

  base::DictionaryValue* rewrite = nullptr;
  if (!server_config->GetDictionary(kRewrite, &rewrite)) {
    LOG(ERROR) << "Server config: Rewrite option is not set";
  }

  if (rewrite) {
    std::string value;
    for (base::DictionaryValue::Iterator it(*rewrite);
         !it.IsAtEnd(); it.Advance()) {
      if (it.value().GetAsString(&value)) {
        AddRewriteRule(it.key(), value);
      }
    }
  }

  std::string log_dir;
  if (server_config->GetString(kLogDir, &log_dir)) {
    log_dir_ = base::FilePath(log_dir);
  }

  return true;
}

bool ServerConfig::ParseCommandLine(const base::CommandLine* command_line) {
  CHECK(command_line);

  if (command_line->HasSwitch(kConfig)) {
    return ParseConfigFile(command_line->GetSwitchValuePath(kConfig));
  }

  daemon_ = command_line->HasSwitch(kDaemon);
  stop_ = command_line->HasSwitch(kStop);
  logging_ = command_line->HasSwitch(kLogging);


  if (command_line->HasSwitch(kCertfile)) {
    certfile_ = command_line->GetSwitchValuePath(kCertfile);
  }

  if (command_line->HasSwitch(kKeyfile)) {
    keyfile_ = command_line->GetSwitchValuePath(kKeyfile);
  }

  if (command_line->HasSwitch(kQuicPort)) {
    int quic_port = 0;
    if (!base::StringToInt(command_line->GetSwitchValueASCII(kQuicPort),
                           &quic_port)) {
      LOG(ERROR) << "--quic_port format is not integer";
      return false;
    }

    if (quic_port <= 0 || quic_port > kUpperBoundPort) {
      LOG(ERROR) << "--quic_port range is invalid";
      return false;
    }

    quic_port_ = quic_port;
  }

  if (command_line->HasSwitch(kProxyTimeout)) {
    if (!base::StringToInt(command_line->GetSwitchValueASCII(kProxyTimeout),
                           &proxy_timeout_)) {
      LOG(ERROR) << "--proxy_timeout is not a digit format";
      return false;
    }

    if (proxy_timeout_ < 0) {
      LOG(ERROR) << "--proxy_timeout range is invalid";
      return false;
    }
  }

  if (command_line->HasSwitch(kSendBufferSize)) {
    if (!base::StringToInt(command_line->GetSwitchValueASCII(kSendBufferSize),
                           &send_buffer_size_)) {
      LOG(ERROR) << "--send_buffer_size is not in a digit format";
      return false;
    }

    if (send_buffer_size_ < 0) {
      LOG(ERROR) << "--send_buffer_size range is invalid";
      return false;
    }
  }

  if (command_line->HasSwitch(kRecvBufferSize)) {
    if (!base::StringToInt(command_line->GetSwitchValueASCII(kRecvBufferSize),
                           &recv_buffer_size_)) {
      LOG(ERROR) << "--recv_buffer_size is not in a digit format";
      return false;
    }

    if (recv_buffer_size_ < 0) {
      LOG(ERROR) << "--recv_buffer_size range is invalid";
      return false;
    }
  }

  if (command_line->HasSwitch(kProxyPass)) {
    std::string proxy_pass_url = command_line->GetSwitchValueASCII(kProxyPass);
    if (!proxy_pass(proxy_pass_url)) {
      LOG(ERROR) << "proxy_pass URL scheme is invalid";
      return false;
    }
  }

  if (command_line->HasSwitch(kBindAddress)) {
    bind_address_ = command_line->GetSwitchValueASCII(kBindAddress);
  }

  if (command_line->HasSwitch(kWorkerCount)) {
    if (!base::StringToInt(command_line->GetSwitchValueASCII(kWorkerCount),
                           &worker_count_)) {
      LOG(ERROR) << "worker_count is not in a valid digit format";
      return false;
    }

    if (worker_count_ <= 0) {
      LOG(ERROR) << "--worker_count range is invalid";
      return false;
    }
  }

  if (command_line->HasSwitch(kDispatchContinuity)) {
    if (!base::StringToInt(
            command_line->GetSwitchValueASCII(kDispatchContinuity),
            &dispatch_continuity_)) {
      LOG(ERROR) << "dispatch_continuity is not in a valid digit format";
      return false;
    }

    if (dispatch_continuity_ <= 0) {
      LOG(ERROR) << "--dispatch_continuity range is invalid";
      return false;
    }
  }

  if (logging_ && command_line->HasSwitch(kLogDir)) {
    log_dir_ = command_line->GetSwitchValuePath(kLogDir);
  }

  return true;
}

// Rewrite rule
bool ServerConfig::AddRewriteRule(const std::string& pattern,
                                  const std::string& replace_format) {
  rewrite_rules_.push_back(std::make_pair(pattern, replace_format));
  return true;
}

} // namespace net
