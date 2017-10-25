//
// Copyright 2016 LINE Corporation
//

#include "base/files/file_path.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "stellite/include/http_client_context.h"
#include "stellite/test/stellite_test_suite.h"
#include "net/base/network_change_notifier.h"
#include "testing/gtest/include/gtest/gtest.h"

StelliteTestSuite::StelliteTestSuite(int argc, char** argv)
    : TestSuite(argc, argv) {
}

StelliteTestSuite::~StelliteTestSuite() {}

void StelliteTestSuite::Initialize() {
  TestSuite::Initialize();

  InitializeInternal();
}

void StelliteTestSuite::Shutdown() {
  TeardownInternal();

  TestSuite::Shutdown();
}

void StelliteTestSuite::InitializeInternal() {
  network_change_notifier_.reset(net::NetworkChangeNotifier::CreateMock());

  host_resolver_proc_ = new net::RuleBasedHostResolverProc(NULL);
  host_resolver_proc_->AddRule("*", "127.0.0.1");

  scoped_host_resolver_proc_.Init(host_resolver_proc_.get());

  message_loop_.reset(new base::MessageLoopForIO());
}

void StelliteTestSuite::TeardownInternal() {
  network_change_notifier_.reset();
  message_loop_.reset();
}

