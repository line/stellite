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

#ifndef STELLITE_INCLUDE_STELLITE_EXPORT_H_
#define STELLITE_INCLUDE_STELLITE_EXPORT_H_

// Setting a visibility of functions and classes

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(STELLITE_IMPLEMENTATION)
#define STELLITE_EXPORT __declspec(dllexport)
#else
#define STELLITE_EXPORT __declspec(dllimport)
#endif  // defined(STELLITE_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(STELLITE_IMPLEMENTATION)
#define STELLITE_EXPORT __attribute__((visibility("default")))
#else
#define STELLITE_EXPORT
#endif
#endif

#else  /// defined(COMPONENT_BUILD)
#define STELLITE_EXPORT
#endif


#endif  // STELLITE_INCLUDE_STELLITE_EXPORT_H_
