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

#include "stellite/process/daemon.h"

#include <fcntl.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/wait.h>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_file.h"
#include "base/strings/string_number_conversions.h"

namespace net {

const char* kNullDevice = "/dev/null";

Daemon::Daemon() {}

Daemon::~Daemon() {}

bool Daemon::Initialize(const std::string& pid_path) {
  pid_path_ = pid_path;
  return true;
}

bool Daemon::Daemonize() {
  if (!CheckPid()) {
    return false;
  }

  // Double-fork to prevent dangling
  for (int i = 0; i < 2; ++i) {
    int pid = fork();
    if (pid < 0) {
      LOG(ERROR) << "Fork error";
      return false;
    }

    // Exit the parent process for daemonizing
    if (pid > 0) {
      exit(0);
    }
  }

  if (setsid() < 0) {
    LOG(ERROR) << "Failed to create a new session";
    return false;
  }


  base::ScopedFD null_device_fd(open(kNullDevice, O_RDWR));
  if (null_device_fd.get() < 0) {
    LOG(ERROR) << "Failed to redirect standard output: " << kNullDevice;
    return false;
  }

  dup2(null_device_fd.get(), STDIN_FILENO);
  dup2(null_device_fd.get(), STDOUT_FILENO);
  dup2(null_device_fd.get(), STDERR_FILENO);

  return CreatePidFile();
}

bool Daemon::Kill() {
  int pid = -1;
  if (!ReadPidFile(&pid)) {
    return false;
  }
  kill(pid, SIGQUIT);
  return true;
}

base::ScopedFD Daemon::LockAndGetPidFile() {
  base::ScopedFD pid_fd(open(pid_path_.c_str(), O_RDWR | O_CREAT, 0600));
  if (pid_fd.get() < 0) {
    LOG(ERROR) << "Cannot open the PID file: " << pid_path_;
    return base::ScopedFD();
  }

  int ret = flock(pid_fd.get(), LOCK_EX | LOCK_NB);
  if (ret < 0) {
    if (errno == EWOULDBLOCK) {
      LOG(ERROR) << "Server process is already running";
    } else {
      LOG(ERROR) << "The PID file is locked: " << pid_path_;
    }
    return base::ScopedFD();
  }

  return pid_fd;
}

bool Daemon::CheckPid() {
  base::ScopedFD pid_fd(LockAndGetPidFile());
  return pid_fd.get() != base::ScopedFD::traits_type::InvalidValue();
}

bool Daemon::CreatePidFile() {
  base::ScopedFD pid_fd(LockAndGetPidFile());

  struct stat pid_stat;
  if (fstat(pid_fd.get(), &pid_stat) == -1) {
    LOG(ERROR) << "Cannot stat the PID file";
    return false;
  }

  if (pid_stat.st_size != 0) {
    if (ftruncate(pid_fd.get(), pid_stat.st_size) == -1) {
      LOG(ERROR) << "Cannot truncate the PID file: " << pid_path_;
      return false;
    }
  }

  char pid_str[16] = {0, };
  snprintf(pid_str, sizeof(pid_str), "%d", getpid());
  int bytes = static_cast<int>(strlen(pid_str));
  if (write(pid_fd.get(), pid_str, strlen(pid_str)) != bytes) {
    LOG(ERROR) << "Cannot write the PID file: " << pid_path_;
    return false;
  }

  // Keep the PID file locked
  pid_fd_.swap(pid_fd);

  return true;
}

bool Daemon::ReadPidFile(int* pid) {
  base::ScopedFD pid_fd(open(pid_path_.c_str(), O_RDONLY));
  if (pid_fd.get() < 0) {
    LOG(ERROR) << "PID file does not exist. Please check if daemon is running";
    return false;
  }

  char pid_str[16] = {0, };
  if (read(pid_fd.get(), pid_str, arraysize(pid_str)) < 0) {
    LOG(ERROR) << "Cannot read the PID file: " << pid_path_;
    return false;
  }

  if (!base::StringToInt(pid_str, pid)) {
    LOG(ERROR) << "The PID format is not integer";
    return false;
  }

  return true;
}

} // namespace net
