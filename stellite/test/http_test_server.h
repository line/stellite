// Copyright 2016 LINE Corporation
//
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUIC_TEST_TOOLS_HTTP_TEST_SERVER_H_
#define QUIC_TEST_TOOLS_HTTP_TEST_SERVER_H_

#include "base/files/scoped_file.h"
#include "base/process/process.h"
#include "base/process/process_handle.h"
#include "net/base/host_port_pair.h"
#include "net/server/http_server.h"

namespace base {
class Thread;
} // namespace base

// HttpTestServer runs an external nodejs-based HTTP server on the same
// machine.
class HttpTestServer {
 public:
  enum ServerType {
    TYPE_HTTP,
    TYPE_HTTPS,
    TYPE_SPDY31,
    TYPE_HTTP2,
    TYPE_QUIC,
    TYPE_PROXY,
  };

  enum RequestType {
    GET,
    POST,
    PUT,
    DELETE,
  };

  enum Platform {
    UNDEFINED,
    LINUX,
    DARWIN,
  };

  struct Params {
    Params();
    ~Params();

    ServerType server_type;
    Platform platform;
    uint16_t port;
    std::string alternative_services;
    uint32_t response_delay;
  };

  HttpTestServer(const Params& params);
  ~HttpTestServer();

  bool Start();

  bool Shutdown();

  bool StartInBackground();

 private:
  bool LaunchNodejs(const base::FilePath& testserver_path);

  bool WaitToStart();

  uint16_t GetPort();

  Params params_;

  base::Process process_;

  base::ScopedFD child_fd_;

  net::HostPortPair host_port_pair_;

  DISALLOW_COPY_AND_ASSIGN(HttpTestServer);
};


#endif // QUIC_TEST_TOOLS_HTTP_TEST_SERVER_H_
