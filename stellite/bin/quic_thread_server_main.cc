//
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

#include <memory>
#include <vector>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/quic/core/crypto/crypto_server_config_protobuf.h"
#include "net/quic/core/quic_protocol.h"
#include "stellite/process/daemon.h"
#include "stellite/server/quic_thread_server.h"

const char* kDefaultPidFilePath = "/tmp/quic.pid";

int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();

  net::ServerConfig server_config;
  if (command_line->HasSwitch("h") || command_line->HasSwitch("help")) {
    server_config.PrintHelpMessages();
    exit(0);
  }

  if (!server_config.ParseCommandLine(command_line)) {
    LOG(ERROR) << "Failed to parse the command line";
    exit(1);
  }

  base::FilePath log_dir = server_config.log_dir();
  if (!server_config.logging() && log_dir.empty()) {
    LOG(WARNING) << "log_dir was ignored because logging is turned off";
  }

  std::unique_ptr<net::Daemon> daemon(new net::Daemon());
  daemon->Initialize(kDefaultPidFilePath);
  if (server_config.stop()) {
    daemon->Kill();
    return 0;
  }

  net::IPAddress bind_address;
  if (!bind_address.AssignFromIPLiteral(server_config.bind_address())) {
    LOG(ERROR) << "Cannot parse the IP address for binding:"
        << server_config.bind_address();
    exit(1);
  }

  if (server_config.keyfile().empty()) {
    LOG(ERROR) << "--keyfile is missing";
    exit(1);
  }

  if (server_config.certfile().empty()) {
    LOG(ERROR) << "--certfile is missing";
    exit(1);
  }

  // Daemonize
  if (server_config.daemon()) {
    if (!daemon->Daemonize()) {
      exit(1);
    }
  }

  base::AtExitManager exit_manager;
  base::MessageLoopForIO message_loop;

  if (log_dir.empty()) {
    log_dir = command_line->GetProgram().DirName();
  }

  if (server_config.logging()) {
    logging::LoggingSettings logging_settings;
    logging_settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
    logging::InitLogging(logging_settings);
  }

  net::QuicConfig quic_config;
  std::unique_ptr<net::QuicThreadServer> quic_thread_server(
      new net::QuicThreadServer(quic_config,
                                server_config,
                                net::AllSupportedVersions()));

  quic_thread_server->Initialize();

  LOG(INFO) << "quic server(" << server_config.quic_port() << ") start";

  std::vector<net::QuicServerConfigProtobuf*> serialized_config;
  quic_thread_server->Start(server_config.worker_count(),
                            net::IPEndPoint(bind_address,
                                            server_config.quic_port()),
                            serialized_config);

  base::RunLoop().Run();

  return 0;
}
