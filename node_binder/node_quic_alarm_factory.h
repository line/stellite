// Copyright 2017 LINE Corporation
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

#ifndef NODE_BINDER_NODE_QUIC_ALARM_FACTORY_H_
#define NODE_BINDER_NODE_QUIC_ALARM_FACTORY_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "net/quic/core/quic_alarm_factory.h"
#include "net/quic/core/quic_clock.h"
#include "net/quic/core/quic_time.h"
#include "stellite/include/stellite_export.h"

namespace stellite {

class NodeQuicAlarmFactory : public net::QuicAlarmFactory {
 public:
  NodeQuicAlarmFactory(const net::QuicClock* clock);
  ~NodeQuicAlarmFactory() override;

  // QuicAlarmFactory implements
  net::QuicAlarm* CreateAlarm(net::QuicAlarm::Delegate* delegate) override;

  net::QuicArenaScopedPtr<net::QuicAlarm> CreateAlarm(
      net::QuicArenaScopedPtr<net::QuicAlarm::Delegate> delegate,
      net::QuicConnectionArena* arena) override;

 private:
  const net::QuicClock* clock_;
  base::WeakPtrFactory<NodeQuicAlarmFactory> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(NodeQuicAlarmFactory);
};

}  // namespace stellite

#endif  // NODE_BINDER_NODE_QUIC_ALARM_FACTORY_H_
