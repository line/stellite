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

#include "stellite/client/backend_cache.h"

#include "net/base/net_errors.h"
#include "net/disk_cache/disk_cache.h"
#include "net/log/net_log.h"
#include "stellite/client/backend_factory.h"


namespace net {

BackendCache::BackendCache(NetLog* net_log, BackendFactory* factory)
    : net_log_(net_log),
      backend_factory_(factory),
      disk_cache_(nullptr),
      weak_factory_(this) {
  DCHECK(net_log);
  DCHECK(factory);
}

BackendCache::~BackendCache() {
  Close();
}

int BackendCache::GetBackend(disk_cache::Backend** backend,
                             const CompletionCallback& callback) {
  if (disk_cache_.get()) {
    *backend = disk_cache_.get();
    return OK;
  }

  return CreateBackend(backend, callback);
}

void BackendCache::OnBackendCreated(disk_cache::Backend** backend,
                                    const CompletionCallback& callback,
                                    int rv) {
  if (rv != OK) {
    return;
  }

  if (disk_cache_.get()) {
    *backend = disk_cache_.get();
  }

  callback.Run(rv);
}

int BackendCache::CreateBackend(disk_cache::Backend** backend,
                                const CompletionCallback& callback) {
  if (!backend_factory_) {
    return ERR_FAILED;
  }

  int rv = backend_factory_->CreateBackend(net_log_, &disk_cache_,
      base::Bind(&BackendCache::OnBackendCreated,
                 weak_factory_.GetWeakPtr(),
                 backend,
                 callback));

  if (rv == net::OK) {
    *backend = disk_cache_.get();
  }

  return rv;
}

void BackendCache::Close() {
  if (!disk_cache_) {
    return;
  }
}

} // namespace net
