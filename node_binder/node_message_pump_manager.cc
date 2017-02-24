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

#include "node_binder/node_message_pump_manager.h"

#include "base/lazy_instance.h"
#include "base/threading/thread_local.h"
#include "node_binder/base/message_pump_node.h"

namespace stellite {

namespace {

base::LazyInstance<base::ThreadLocalPointer<NodeMessagePumpManager>>::Leaky
    tls_ptr = LAZY_INSTANCE_INITIALIZER;

}  // namespace anonymous

//  static
NodeMessagePumpManager* NodeMessagePumpManager::current() {
  return tls_ptr.Pointer()->Get();
}

NodeMessagePumpManager::NodeMessagePumpManager() {
  DCHECK_EQ(tls_ptr.Pointer()->Get(), nullptr);
  tls_ptr.Pointer()->Set(this);
}

NodeMessagePumpManager::~NodeMessagePumpManager() {
  DCHECK_EQ(tls_ptr.Pointer()->Get(), this);
  tls_ptr.Pointer()->Set(nullptr);
}

void NodeMessagePumpManager::AddRef() {
  DCHECK(CalledOnValidThread());

  DCHECK_GE(ref_count_, 0);

  if (ref_count_ == 0) {
    base::MessagePumpNode::current()->StartAsyncTask();
  }

  ++ref_count_;
}

void NodeMessagePumpManager::RemoveRef() {
  DCHECK(CalledOnValidThread());
  DCHECK_GT(ref_count_, 0);
  --ref_count_;

  if (ref_count_ == 0) {
    base::MessagePumpNode::current()->StopAsyncTask();
  }
}

}  // namespace stellite
