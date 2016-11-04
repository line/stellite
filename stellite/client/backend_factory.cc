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

#include "stellite/client/backend_factory.h"

#include "base/single_thread_task_runner.h"
#include "net/base/cache_type.h"
#include "net/disk_cache/disk_cache.h"
#include "net/log/net_log.h"

namespace net {

BackendFactory::BackendFactory(
    CacheType type,
    BackendType backend_type,
    const base::FilePath& path,
    int max_bytes,
    const scoped_refptr<base::SingleThreadTaskRunner>& thread)
    : type_(type),
      backend_type_(backend_type),
      path_(path),
      max_bytes_(max_bytes),
      thread_(thread) {
}

BackendFactory::~BackendFactory() {}

int BackendFactory::CreateBackend(NetLog* net_log,
                                  std::unique_ptr<disk_cache::Backend>* backend,
                                  const CompletionCallback& callback) {
  return disk_cache::CreateCacheBackend(type_,
                                        backend_type_,
                                        path_,
                                        max_bytes_,
                                        /* force*/ true,
                                        thread_,
                                        net_log,
                                        backend,
                                        callback);
}

// static
BackendFactory* BackendFactory::InMemory(int max_bytes) {
  return new BackendFactory(MEMORY_CACHE, CACHE_BACKEND_DEFAULT,
                            base::FilePath(), max_bytes, NULL);
}

} // namespace net
