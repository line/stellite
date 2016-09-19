//
// Copyright 2016 LINE Corporation
//

#include <memory>
#include <vector>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "stellite/crypto/quic_ephemeral_key_source.h"
#include "stellite/logging/logging.h"
#include "stellite/logging/logging_service.h"
#include "stellite/process/daemon.h"
#include "stellite/server/quic_thread_server.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/quic/crypto/crypto_server_config_protobuf.h"
#include "net/quic/crypto/proof_source_chromium.h"
#include "net/quic/quic_protocol.h"

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
    stellite::LoggingService* logging_service =
        stellite::LoggingService::GetInstance();
    if (!logging_service->Initialize(log_dir)) {
      exit(1);
    }
  }

  net::ProofSourceChromium* proof_source = new net::ProofSourceChromium();
  if (!proof_source->Initialize(server_config.certfile(),
                                server_config.keyfile(),
                                base::FilePath())) {
    FLOG(ERROR) << "Failed to parse the certificate";
    exit(1);
  }

  net::QuicConfig quic_config;
  std::unique_ptr<net::QuicThreadServer> quic_thread_server(
      new net::QuicThreadServer(quic_config,
                                server_config,
                                net::QuicSupportedVersions(),
                                proof_source));

  std::vector<net::QuicServerConfigProtobuf*> quic_server_configs;
  if (!quic_thread_server->Initialize(quic_server_configs)) {
    FLOG(ERROR) << "Failed to initialize the QUIC server";
    exit(1);
  }

  // Ephemeral key source, owned by server
  quic_thread_server->SetEphemeralKeySource(new net::QuicEphemeralKeySource());

  FLOG(INFO) << "quic server(" << server_config.quic_port() << ") start";

  quic_thread_server->Start(server_config.worker_count(),
                            net::IPEndPoint(bind_address,
                                            server_config.quic_port()));

  message_loop.Run();

  return 0;
}