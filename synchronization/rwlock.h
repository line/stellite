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

#ifndef STELLITE_SYNCHRONIZATION_RWLOCK_H_
#define STELLITE_SYNCHRONIZATION_RWLOCK_H_

#include "base/macros.h"
#include "net/base/net_export.h"

#include <pthread.h>

namespace net {

// RWLock is a read and write lock. It is supported only on POSIX
class NET_EXPORT RWLock {
 public:
  RWLock();
  ~RWLock();

  bool TryLockShared();

  // If a lock is not held, takes a lock and returns true. If a lock is already
  // taken, immediately returns false.
  bool TryLockExclusive();

  // Acquires a read lock if a write lock has been acquired by another thread.
  // This will be block until the write lock is released.
  void AcquireLockShared();

  // Acquires a write lock
  void AcquireLockExclusive();

  void ReleaseLock();

 private:
  pthread_rwlock_t rwlock_;

  DISALLOW_COPY_AND_ASSIGN(RWLock);
};

class NET_EXPORT ReadLockScoped {
 public:
  explicit ReadLockScoped(RWLock& lock);
  ~ReadLockScoped();

 private:
  RWLock& lock_;

  DISALLOW_COPY_AND_ASSIGN(ReadLockScoped);
};

class NET_EXPORT WriteLockScoped {
 public:
  explicit WriteLockScoped(RWLock& lock);
  ~WriteLockScoped();

 private:
  RWLock& lock_;

  DISALLOW_COPY_AND_ASSIGN(WriteLockScoped);
};

}  // namespace net

#endif // STELLITE_SYNCHRONIZATION_RWLOCK_H_
