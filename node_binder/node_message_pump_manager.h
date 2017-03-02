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

#ifndef NODE_BINDER_NODE_MESSAGE_PUMP_MANAGER_H_
#define NODE_BINDER_NODE_MESSAGE_PUMP_MANAGER_H_

#include <memory>

#include "base/macros.h"
#include "base/threading/non_thread_safe.h"

namespace stellite {

class NodeMessagePumpManager : public base::NonThreadSafe {
 public:
  static NodeMessagePumpManager* current();

  NodeMessagePumpManager();
  ~NodeMessagePumpManager();

  void AddRef();
  void RemoveRef();

 private:
  int ref_count_;

  DISALLOW_COPY_AND_ASSIGN(NodeMessagePumpManager);
};

}  // namespace stellite

#endif  // NODE_BINDER_NODE_MESSAGE_PUMP_MANAGER_H_
