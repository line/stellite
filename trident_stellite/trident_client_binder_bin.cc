//
  //void* context = new_context();
// 2016 write by snibug@linecorp.com
//

#include <iostream>
#include <sstream>

#include "stellite/stub/client_binder.h"

const char* kDefaultQuicHost = "perf-quic1.line-apps-beta.com:443";
const char* kExitCommand = "exit";

inline std::string trim(const std::string& src) {
  std::stringstream stream;
  stream << src;
  return stream.str();
}

int main(int argc, char* argv[]) {
  void* context = new_context_with_quic_host(kDefaultQuicHost);
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
