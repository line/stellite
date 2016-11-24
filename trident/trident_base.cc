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

#include "trident_stellite/trident_base.h"

#include "base/command_line.h"

namespace trident {

TridentBase::TridentBase() {}

TridentBase::~TridentBase() {}

void TridentBase::Initialize(int argc, char *argv[]) {
  base::CommandLine::Init(argc, argv);
}

}  // namespace trident
