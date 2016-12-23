// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef STELLITE_TEST_STELLITE_TEST_SUITE_H_
#define STELLITE_TEST_STELLITE_TEST_SUITE_H_

#include <memory>

#include "base/memory/ref_counted.h"
#include "base/test/test_suite.h"
#include "net/dns/mock_host_resolver.h"

namespace base {
class MessageLoop;
} // namespace base

namespace net {
class NetworkChangeNotifier;
} // namespace net

class StelliteTestSuite : public base::TestSuite {
 public:
  StelliteTestSuite(int argc, char** argv);
  ~StelliteTestSuite() override;

  void Initialize() override;
  void Shutdown() override;

 protected:
  void InitializeInternal();

 private:
  std::unique_ptr<net::NetworkChangeNotifier> network_change_notifier_;
  std::unique_ptr<base::MessageLoop> message_loop_;
  scoped_refptr<net::RuleBasedHostResolverProc> host_resolver_proc_;
  net::ScopedDefaultHostResolverProc scoped_host_resolver_proc_;
};


#endif // STELLITE_TEST_STELLITE_TEST_SUITE_H_
