//
// Copyright 2016 LINE Corporation
//

#include "base/files/file_path.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "stellite/include/http_client_context.h"
#include "stellite/test/quic_test_suite.h"
#include "net/base/network_change_notifier.h"
#include "testing/gtest/include/gtest/gtest.h"

QuicTestSuite::QuicTestSuite(int argc, char** argv)
    : TestSuite(argc, argv) {
}

QuicTestSuite::~QuicTestSuite() {}

void QuicTestSuite::Initialize() {
  TestSuite::Initialize();
  InitializeInternal();
}

void QuicTestSuite::Shutdown() {
  message_loop_.reset();

  TestSuite::Shutdown();
}

void QuicTestSuite::InitializeInternal() {
  network_change_notifier_.reset(net::NetworkChangeNotifier::CreateMock());

  host_resolver_proc_ = new net::RuleBasedHostResolverProc(NULL);
  scoped_host_resolver_proc_.Init(host_resolver_proc_.get());
  host_resolver_proc_->AddRule("*", "127.0.0.1");

#if defined(USE_OPENSSL_CERTS)
  base::FilePath root;
  if (!PathService::Get(base::DIR_SOURCE_ROOT, &root)) {
    LOG(WARNING) << "Failed to get the Chromium root path";
  }

  base::FilePath cacert = root.
      Append(FILE_PATH_LITERAL("stellite")).
      Append(FILE_PATH_LITERAL("test_tools")).
      Append(FILE_PATH_LITERAL("res")).
      Append(FILE_PATH_LITERAL("example.com.cacert.pem"));
  if (!stellite::HttpClientContext::ResetCertBundle(cacert.value())) {
    LOG(WARNING) << "Failed to load the root certificate: " << cacert.value();
  }
#endif // USE_OPENSSL_CERTS

  message_loop_.reset(new base::MessageLoopForIO());
}