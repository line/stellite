// Copyright 2016 LINE Corporation
//
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stellite/test/http_test_server.h"

#include <poll.h>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/posix/eintr_wrapper.h"
#include "base/process/kill.h"
#include "base/process/launch.h"
#include "base/process/process_iterator.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/test_timeouts.h"

namespace {

const char kPlatformLinux[] = "linux";
const char kPlatformDarwin[] = "darwin";

bool ReadData(int fd, ssize_t bytes_max, uint8_t* buffer,
              base::TimeDelta* remaining_time) {
  ssize_t bytes_read = 0;
  base::TimeTicks previous_time = base::TimeTicks::Now();
  while (bytes_read < bytes_max) {
    struct pollfd poll_fds[1];

    poll_fds[0].fd = fd;
    poll_fds[0].events = POLLIN;
    poll_fds[0].revents = 0;

    int rv = HANDLE_EINTR(poll(poll_fds, 1,
                               remaining_time->InMilliseconds()));
    if (rv == 0) {
      LOG(ERROR) << "poll(" << fd << ") timed out; bytes_read=" << bytes_read;
      return false;
    } else if (rv < 0) {
      PLOG(ERROR) << "poll() failed for child file descriptor; bytes_read="
                  << bytes_read;
      return false;
    }

    base::TimeTicks current_time = base::TimeTicks::Now();
    base::TimeDelta elapsed_time_cycle = current_time - previous_time;
    DCHECK_GE(elapsed_time_cycle.InMilliseconds(), 0);
    *remaining_time -= elapsed_time_cycle;
    previous_time = current_time;

    ssize_t num_bytes = HANDLE_EINTR(read(fd, buffer + bytes_read,
                                          bytes_max - bytes_read));
    if (num_bytes <= 0) {
      return false;
    }
    bytes_read += num_bytes;
  }

  return true;
}

} // namespace anonymous

class OrphanedTestServerFilter : public base::ProcessFilter {
 public:
  OrphanedTestServerFilter(const std::string& path_string,
                           const std::string& port_string)
      : path_string_(path_string),
        port_string_(port_string) {}

  bool Includes(const base::ProcessEntry& entry) const override {
    if (entry.parent_pid() != 1) {
      return false;
    }

    bool found_path_string = false;
    bool found_port_string = false;
    std::vector<std::string>::const_iterator it = entry.cmd_line_args().begin();
    while (it != entry.cmd_line_args().end()) {
      if (it->find(path_string_) != std::string::npos) {
        found_path_string = true;
      }
      if (it->find(port_string_) != std::string::npos) {
        found_port_string = true;
      }
      ++it;
    }
    return found_path_string && found_port_string;
  }

 private:
  std::string path_string_;
  std::string port_string_;

  DISALLOW_COPY_AND_ASSIGN(OrphanedTestServerFilter);
};

HttpTestServer::Params::Params()
  : server_type(TYPE_HTTP),
    platform(UNDEFINED),
    port(0),
    response_delay(0) {
}

HttpTestServer::Params::~Params() {
}

HttpTestServer::HttpTestServer(const Params& params)
    : params_(params) {
#ifdef OS_MACOS
  params_.platform = DARWIN;
#elif OS_LINUX
  params_.platform = LINUX;
#endif
}

HttpTestServer::~HttpTestServer() {}

uint16_t HttpTestServer::GetPort() {
  return host_port_pair_.port();
}

bool HttpTestServer::Start() {
  if (params_.port <= 0) {
    LOG(ERROR) << "The port number is not set";
    return false;
  }
  host_port_pair_.set_port(params_.port);
  return StartInBackground() && WaitToStart();
}

bool HttpTestServer::Shutdown() {
  // Kill
  process_.Terminate(0, false);

  // Wait for exit
  int exit_code = 0;
  base::TimeDelta remaining_time = TestTimeouts::action_timeout();
  process_.WaitForExitWithTimeout(remaining_time, &exit_code);

  return exit_code == 0;
}

bool HttpTestServer::StartInBackground() {
  base::FilePath testserver_path;
  if (!PathService::Get(base::DIR_SOURCE_ROOT, &testserver_path)) {
    LOG(ERROR) << "Failed to get DIR_SOURCE_ROOT";
    return false;
  }

  testserver_path = testserver_path.Append(FILE_PATH_LITERAL("stellite")).
                                    Append(FILE_PATH_LITERAL("test_tools")).
                                    Append(FILE_PATH_LITERAL("node")).
                                    Append(FILE_PATH_LITERAL("testserver.js"));

  if (!LaunchNodejs(testserver_path)) {
    return false;
  }

  return true;
}

bool HttpTestServer::LaunchNodejs(const base::FilePath& testserver_path) {
  // Kill an orphan testserver process if the testserver is alive
  OrphanedTestServerFilter filter(testserver_path.value(),
                                  base::IntToString(GetPort()));
  base::KillProcesses("node", -1, &filter);

  base::FilePath root_path;
  if (!PathService::Get(base::DIR_SOURCE_ROOT, &root_path)) {
    LOG(ERROR) << "Fail to get base::DIR_SOURCE_ROOT";
    return false;
  }

  base::FilePath node_path = root_path.
      Append(FILE_PATH_LITERAL("stellite")).
      Append(FILE_PATH_LITERAL("test_tools")).
      Append(FILE_PATH_LITERAL("node")).
      Append(FILE_PATH_LITERAL(params_.platform == LINUX ? kPlatformLinux :
                               kPlatformDarwin)).
      Append(FILE_PATH_LITERAL("bin")).
      Append(FILE_PATH_LITERAL("node"));

  base::CommandLine node_command(node_path);

  // Append node arguments
  node_command.AppendArgPath(testserver_path);

  // Server type
  switch (params_.server_type) {
    case TYPE_HTTP:
      node_command.AppendArg("--http");
      break;
    case TYPE_HTTPS:
      node_command.AppendArg("--https");
      break;
    case TYPE_SPDY31:
      node_command.AppendArg("--spdy");
      break;
    case TYPE_HTTP2:
      node_command.AppendArg("--http2");
      break;
    case TYPE_PROXY:
      node_command.AppendArg("--proxy");
      break;
    default:
      LOG(ERROR) << "Unknown server type";
      break;
  }

  if (params_.alternative_services.size()) {
    node_command.AppendArg("--alternative_services=" +
                           params_.alternative_services);
  }

  // Port
  node_command.AppendArg("--port=" + base::IntToString(GetPort()));

  // Bind pipe
  int pipefd[2];
  if (pipe(pipefd) != 0) {
    LOG(ERROR) << "Cannot create pipe";
    return false;
  }

  child_fd_.reset(pipefd[0]);
  base::ScopedFD write_closer(pipefd[1]);
  base::FileHandleMappingVector map_write_fd;
  map_write_fd.push_back(std::make_pair(pipefd[1], pipefd[1]));

  node_command.AppendArg("--startup_pipe=" + base::IntToString(pipefd[1]));

  base::LaunchOptions process_options;
  process_options.fds_to_remap = &map_write_fd;

  process_ = base::LaunchProcess(node_command, process_options);
  if (!process_.IsValid()) {
    LOG(ERROR) << "failed to launch " << node_command.GetCommandLineString();
    return false;
  }

  return true;
}

bool HttpTestServer::WaitToStart() {
  base::ScopedFD child_fd(child_fd_.release());
  base::TimeDelta remaining_time = TestTimeouts::action_timeout();

  uint32_t notify_data = 0;
  ReadData(child_fd.get(), sizeof(notify_data),
           reinterpret_cast<uint8_t*>(notify_data),
           &remaining_time);
  return true;
}
