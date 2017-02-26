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

#include "node_binder/node_quic_alarm_factory.h"

#include <memory>

#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/quic/core/quic_alarm.h"
#include "net/quic/core/quic_clock.h"
#include "node/uv.h"

namespace stellite {

namespace {

class NodeQuicAlarm : public net::QuicAlarm {
 public:
  NodeQuicAlarm(const net::QuicClock* clock,
                net::QuicArenaScopedPtr<Delegate> delegate);
  ~NodeQuicAlarm() override;

 protected:
  void SetImpl() override;
  void CancelImpl() override;

 private:
  void OnAlarm();

  const net::QuicClock* clock_;
  net::QuicTime task_deadline_;
  base::WeakPtrFactory<NodeQuicAlarm> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(NodeQuicAlarm);
};

NodeQuicAlarm::NodeQuicAlarm(const net::QuicClock* clock,
                             net::QuicArenaScopedPtr<Delegate> delegate)
    : QuicAlarm(std::move(delegate)),
      clock_(clock),
      task_deadline_(net::QuicTime::Zero()),
      weak_factory_(this) {}

NodeQuicAlarm::~NodeQuicAlarm() {}

void NodeQuicAlarm::SetImpl() {
  LOG(INFO) << "NodeQuicAlarm::SetImpl()";
  if (task_deadline_.IsInitialized()) {
    if (task_deadline_ <= deadline()) {
      // Since tasks can not be un-posted, OnAlarm will be invoked which
      // will notice that deadline has not yet been reached, and will set
      // the alarm for the new deadline.
      return;
    }
    // The scheduled task is after new deadline.  Invalidate the weak ptrs
    // so that task does not execute when we're not expecting it.
    weak_factory_.InvalidateWeakPtrs();
  }

  int64_t delay_us = (deadline() - (clock_->Now())).ToMicroseconds();
  if (delay_us < 0) {
    delay_us = 0;
  }

  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&NodeQuicAlarm::OnAlarm, weak_factory_.GetWeakPtr()),
      base::TimeDelta::FromMicroseconds(delay_us));
  task_deadline_ = deadline();
}

void NodeQuicAlarm::CancelImpl() {
  DCHECK(!deadline().IsInitialized());
}

void NodeQuicAlarm::OnAlarm() {
  DCHECK(task_deadline_.IsInitialized());
  task_deadline_ = net::QuicTime::Zero();
  if (!deadline().IsInitialized()) {
    return;
  }

  if (clock_->Now() < deadline()) {
    SetImpl();
    return;
  }

  Fire();
}

}  // namespace anonymouse

NodeQuicAlarmFactory::NodeQuicAlarmFactory(const net::QuicClock* clock)
    : clock_(clock),
      weak_factory_(this) {
}

NodeQuicAlarmFactory::~NodeQuicAlarmFactory() {}

net::QuicAlarm* NodeQuicAlarmFactory::CreateAlarm(net::QuicAlarm::Delegate* delegate) {
  return new NodeQuicAlarm(
      clock_,
      net::QuicArenaScopedPtr<net::QuicAlarm::Delegate>(delegate));
}

net::QuicArenaScopedPtr<net::QuicAlarm> NodeQuicAlarmFactory::CreateAlarm(
    net::QuicArenaScopedPtr<net::QuicAlarm::Delegate> delegate,
    net::QuicConnectionArena* arena) {
  if (arena != nullptr) {
    return arena->New<NodeQuicAlarm>(clock_, std::move(delegate));
  } else {
    return net::QuicArenaScopedPtr<net::QuicAlarm>(
        new NodeQuicAlarm(clock_, std::move(delegate)));
  }
}

}  // namespace net
