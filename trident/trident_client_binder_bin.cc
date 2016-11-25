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

#include <iostream>
#include <sstream>

#include "stellite/stub/client_binder.h"

const char* kExitCommand = "exit";

inline std::string trim(const std::string& src) {
  std::stringstream stream;
  stream << src;
  return stream.str();
}

int main(int argc, char* argv[]) {
  void* context = new_context();
  void* client = new_client(context);

  while (true) {
    std::string command;
    std::getline(std::cin, command);

    command = trim(command);
    if (command.size() == 0) {
      continue;
    }

    if (command == kExitCommand) {
      break;
    }

    void* response = get(context, client, const_cast<char*>(command.c_str()));
    if (!response) {
      std::cout << "cannot receive response" << std::endl;
      continue;
    }

    std::cout << response_code(response) << std::endl;
    std::cout << response_body(response) << std::endl << std::endl;

    release_response(context, response);
  }

  release_client(context, client);
  release_context(context);

  return 0;
}
