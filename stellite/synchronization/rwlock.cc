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

#include "stellite/synchronization/rwlock.h"

#include <errno.h>
#include <string.h>

#include "base/logging.h"

namespace net {

RWLock::RWLock() {
  pthread_rwlock_init(&rwlock_, nullptr);
}

RWLock::~RWLock() {
  int rv = pthread_rwlock_destroy(&rwlock_);
  DCHECK_EQ(rv, 0) << strerror(rv);
}

bool RWLock::TryLockShared() {
  int rv = pthread_rwlock_tryrdlock(&rwlock_);
  DCHECK(rv == 0 || rv == EBUSY) << strerror(rv);
  return rv == 0;
}

bool RWLock::TryLockExclusive() {
  int rv = pthread_rwlock_trywrlock(&rwlock_);
  DCHECK(rv == 0 || rv == EBUSY) << strerror(rv);
  return rv == 0;
}

void RWLock::AcquireLockShared() {
  int rv = pthread_rwlock_rdlock(&rwlock_);
  DCHECK_EQ(rv, 0) << strerror(rv);
}

void RWLock::AcquireLockExclusive() {
  int rv = pthread_rwlock_wrlock(&rwlock_);
  DCHECK_EQ(rv, 0) << strerror(rv);
}

void RWLock::ReleaseLock() {
  int rv = pthread_rwlock_unlock(&rwlock_);
  DCHECK_EQ(rv, 0) << strerror(rv);
}

ReadLockScoped::ReadLockScoped(RWLock& lock)
  : lock_(lock) {
  lock_.AcquireLockShared();
}

ReadLockScoped::~ReadLockScoped() {
  lock_.ReleaseLock();
}

WriteLockScoped::WriteLockScoped(RWLock& lock)
  : lock_(lock) {
  lock_.AcquireLockExclusive();
}

WriteLockScoped::~WriteLockScoped() {
  lock_.ReleaseLock();
}

}  // namespace net

