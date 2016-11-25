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

#ifndef TRIDENT_TRIDENT_BASE_H_
#define TRIDENT_TRIDENT_BASE_H_

#include <stddef.h>
#include <memory>

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(TRIDENT_IMPLEMENTATION)
#define TRIDENT_EXPORT __declspec(dllexport)
#else
#define TRIDENT_EXPORT __declspec(dllimport)
#endif  // TRIDENT_IMPLEMENTATION

#else  // defined(WIN32)
#if defined(TRIDENT_IMPLEMENTATION)
#define TRIDENT_EXPORT __attribute__((visibility("default")))
#else
#define TRIDENT_EXPORT
#endif  // TRIDENT_IMPLEMENTATION
#endif

#else  // COMPONENT_BUILD
#define TRIDENT_EXPORT
#endif

namespace trident {

class TRIDENT_EXPORT TridentBase {
 public:
  TridentBase();
  ~TridentBase();
  void Initialize(int argc, char *argv[]);
};

}  // namespace trident

#endif  // TRIDENT_TRIDENT_BASE_H_
