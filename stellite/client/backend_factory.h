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

#ifndef STELLITE_CLIENT_BACKEND_FACTORY_H_
#define STELLITE_CLIENT_BACKEND_FACTORY_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "stellite/include/stellite_export.h"
#include "net/base/cache_type.h"
#include "net/base/completion_callback.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace disk_cache {
class Backend;
}

namespace net {
class NetLog;

class STELLITE_EXPORT BackendFactory {
 public:
  BackendFactory(CacheType type,
                 BackendType backend_type,
                 const base::FilePath& path,
                 int max_bytes,
                 const scoped_refptr<base::SingleThreadTaskRunner>& thread);
  virtual ~BackendFactory();

  int CreateBackend(NetLog* net_log,
                    std::unique_ptr<disk_cache::Backend>* backend,
                    const CompletionCallback& callback);

  static BackendFactory* InMemory(int max_bytes);

 private:

  CacheType type_;

  BackendType backend_type_;

  const base::FilePath& path_;

  int max_bytes_;

  scoped_refptr<base::SingleThreadTaskRunner> thread_;

  DISALLOW_COPY_AND_ASSIGN(BackendFactory);
};

} // namespace net

#endif  // STELLITE_CLIENT_BACKEND_FACTORY_H_
