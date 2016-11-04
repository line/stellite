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

#ifndef STELLITE_PROCESS_DAEMON_H_
#define STELLITE_PROCESS_DAEMON_H_

#include <string>

#include "base/files/scoped_file.h"

namespace net {

class Daemon {
 public:
  Daemon();
  virtual ~Daemon();

  bool Initialize(const std::string& pid_path);

  bool Daemonize();

  bool Kill();

 private:
  bool CheckPid();
  base::ScopedFD LockAndGetPidFile();
  bool CreatePidFile();
  bool ReadPidFile(int* pid);

  std::string pid_path_;
  base::ScopedFD pid_fd_;

  DISALLOW_COPY_AND_ASSIGN(Daemon);
};

}// namespace net

#endif // STELLITE_PROCESS_DAEMON_H_
