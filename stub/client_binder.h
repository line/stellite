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

#ifndef STELLITE_STUB_QUIC_PYTHON_BINDER_H_
#define STELLITE_STUB_QUIC_PYTHON_BINDER_H_

#include "stellite/include/stellite_export.h"

// TODO(@snibug): C language interface does work with other language binders
// but they are not mature. We have a goal to use other languages that
// are more instinctive (such as Java, Python, GO).
extern "C" {

STELLITE_EXPORT void* new_context();

STELLITE_EXPORT void* new_context_with_quic();

STELLITE_EXPORT void* new_context_with_quic_host(const char* host,
                                                 bool disk_cache=false);

STELLITE_EXPORT bool release_context(void* raw_context);

STELLITE_EXPORT void* new_client(void* raw_context);

STELLITE_EXPORT bool release_client(void* raw_context, void* raw_client);

STELLITE_EXPORT void* get(void* raw_client, char* url);

STELLITE_EXPORT void* post(void* raw_client, char* url, char* body);

STELLITE_EXPORT void release_response(void* raw_response);

STELLITE_EXPORT int response_code(void* raw_response);

STELLITE_EXPORT const char* response_body(void* raw_response);

// retrive header key and value

STELLITE_EXPORT int get_header_count(void* raw_response);

STELLITE_EXPORT const char* raw_headers(void* raw_response);

STELLITE_EXPORT const char* get_header_key(int pos);

STELLITE_EXPORT const char* get_header_body(int pos);

STELLITE_EXPORT int get_connection_info(void* raw_response);

STELLITE_EXPORT const char* get_connection_info_desc(void* raw_response);

} // extern "C"

#endif
