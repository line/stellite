// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stellite/client/cache_based_quic_server_info.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "base/metrics/histogram.h"
#include "net/base/completion_callback.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/http/http_network_session.h"
#include "net/quic/quic_server_id.h"
#include "stellite/client/backend_cache.h"

namespace net {

// Some APIs inside disk_cache take a handle that the caller must keep alive
// until the API has finished its asynchronous execution.
//
// Unfortunately, CacheBasedQuicServerInfo may be deleted before the
// operation is completed, causing a use-after-free.
//
// This data shim struct is meant to provide a location for the disk_cache
// APIs to write into even if the originating CacheBasedQuicServerInfo
// object has been deleted.  The lifetime of the instances of this struct
// should be bound to the CompletionCallback that is passed to the disk_cache
// API.  We do this by binding an instance of this struct to an unused
// parameter for OnIOComplete() using base::Owned().
//
// This is a hack. A better fix is to make it so that the disk_cache APIs
// take a callback to a mutator for setting the output value rather than
// writing into a raw handle. Then the caller can just pass in a callback
// bound to WeakPtr for itself. This callback would correctly "no-op" itself
// when the CacheBasedQuicServerInfo object is deleted.
//
// TODO(@ajwong): Change disk_cache's API to return results via a callback.
struct CacheBasedQuicServerInfo::CacheOperationDataShim {
  CacheOperationDataShim() : backend(NULL), entry(NULL) {}

  disk_cache::Backend* backend;
  disk_cache::Entry* entry;
};

CacheBasedQuicServerInfo::CacheBasedQuicServerInfo(
    const QuicServerId& server_id,
    BackendCache* backend_cache)
    : QuicServerInfo(server_id),
      data_shim_(new CacheOperationDataShim()),
      state_(GET_BACKEND),
      ready_(false),
      found_entry_(false),
      server_id_(server_id),
      backend_cache_(backend_cache),
      backend_(NULL),
      entry_(NULL),
      last_failure_(NO_FAILURE),
      weak_factory_(this) {
      io_callback_ =
          base::Bind(&CacheBasedQuicServerInfo::OnIOComplete,
                     weak_factory_.GetWeakPtr(),
                     base::Owned(data_shim_));  // Ownership assigned.
}

void CacheBasedQuicServerInfo::Start() {
  DCHECK(CalledOnValidThread());
  DCHECK_EQ(GET_BACKEND, state_);
  DCHECK_EQ(last_failure_, NO_FAILURE);
  RecordQuicServerInfoStatus(QUIC_SERVER_INFO_START);
  load_start_time_ = base::TimeTicks::Now();
  DoLoop(OK);
}

int CacheBasedQuicServerInfo::WaitForDataReady(
    const CompletionCallback& callback) {
  DCHECK(CalledOnValidThread());
  DCHECK_NE(GET_BACKEND, state_);
  wait_for_data_start_time_ = base::TimeTicks::Now();

  RecordQuicServerInfoStatus(QUIC_SERVER_INFO_WAIT_FOR_DATA_READY);
  if (ready_) {
    wait_for_data_end_time_ = base::TimeTicks::Now();
    RecordLastFailure();
    return OK;
  }

  if (!callback.is_null()) {
  // Prevent a new callback for WaitForDataReady from overwriting an existing
    // pending callback (|wait_for_ready_callback_|).
    if (!wait_for_ready_callback_.is_null()) {
      RecordQuicServerInfoFailure(WAIT_FOR_DATA_READY_INVALID_ARGUMENT_FAILURE);
      return ERR_INVALID_ARGUMENT;
    }
    wait_for_ready_callback_ = callback;
  }

  return ERR_IO_PENDING;
}

void CacheBasedQuicServerInfo::ResetWaitForDataReadyCallback() {
  DCHECK(CalledOnValidThread());
  wait_for_ready_callback_.Reset();
}

void CacheBasedQuicServerInfo::CancelWaitForDataReadyCallback() {
  DCHECK(CalledOnValidThread());

  RecordQuicServerInfoStatus(QUIC_SERVER_INFO_WAIT_FOR_DATA_READY_CANCEL);
  if (!wait_for_ready_callback_.is_null()) {
    RecordLastFailure();
    wait_for_ready_callback_.Reset();
  }
}

bool CacheBasedQuicServerInfo::IsDataReady() {
  return ready_;
}

bool CacheBasedQuicServerInfo::IsReadyToPersist() {
  // The data can be persisted if it has been loaded from the disk cache
  // and there are no pending writes.
  RecordQuicServerInfoStatus(QUIC_SERVER_INFO_READY_TO_PERSIST);
  if (ready_ && new_data_.empty())
    return true;
  RecordQuicServerInfoFailure(READY_TO_PERSIST_FAILURE);
  return false;
}

void CacheBasedQuicServerInfo::Persist() {
  DCHECK(CalledOnValidThread());
  if (!IsReadyToPersist()) {
    // Handle updates while a write is pending or if we haven't loaded from disk
    // cache. Save the data to be written into a temporary buffer and then
    // persist that data when we are ready to persist.
    pending_write_data_ = Serialize();
    return;
  }
  PersistInternal();
}

void CacheBasedQuicServerInfo::PersistInternal() {
  DCHECK(CalledOnValidThread());
  DCHECK_NE(GET_BACKEND, state_);
  DCHECK(new_data_.empty());
  CHECK(ready_);
  DCHECK(wait_for_ready_callback_.is_null());

  if (pending_write_data_.empty()) {
    new_data_ = Serialize();
  } else {
    new_data_ = pending_write_data_;
    pending_write_data_.clear();
  }

  RecordQuicServerInfoStatus(QUIC_SERVER_INFO_PERSIST);
  if (!backend_) {
    RecordQuicServerInfoFailure(PERSIST_NO_BACKEND_FAILURE);
    return;
  }

  state_ = CREATE_OR_OPEN;
  DoLoop(OK);
}

void CacheBasedQuicServerInfo::OnExternalCacheHit() {
  DCHECK(CalledOnValidThread());
  DCHECK_NE(GET_BACKEND, state_);

  RecordQuicServerInfoStatus(QUIC_SERVER_INFO_EXTERNAL_CACHE_HIT);
  if (!backend_) {
    RecordQuicServerInfoFailure(PERSIST_NO_BACKEND_FAILURE);
    return;
  }

  backend_->OnExternalCacheHit(key());
}

CacheBasedQuicServerInfo::~CacheBasedQuicServerInfo() {
  DCHECK(wait_for_ready_callback_.is_null());
  if (entry_) {
    entry_->Close();
  }
}

std::string CacheBasedQuicServerInfo::key() const {
  return "quicserverinfo:" + server_id_.ToString();
}

void CacheBasedQuicServerInfo::OnIOComplete(CacheOperationDataShim* unused,
                                            int rv) {
  DCHECK_NE(NONE, state_);
  rv = DoLoop(rv);
  if (rv == ERR_IO_PENDING)
    return;

  base::WeakPtr<CacheBasedQuicServerInfo> weak_this =
      weak_factory_.GetWeakPtr();

  if (!wait_for_ready_callback_.is_null()) {
    wait_for_data_end_time_ = base::TimeTicks::Now();
    RecordLastFailure();
    base::ResetAndReturn(&wait_for_ready_callback_).Run(rv);
  }
  // |wait_for_ready_callback_| can delete the object if there is an error.
  // Check if |weak_this| still exists before accessing it.
  if (weak_this.get() && ready_ && !pending_write_data_.empty()) {
    DCHECK_EQ(NONE, state_);
    PersistInternal();
  }
}

int CacheBasedQuicServerInfo::DoLoop(int rv) {
  do {
    switch (state_) {
      case GET_BACKEND:
        rv = DoGetBackend();
        break;
      case GET_BACKEND_COMPLETE:
        rv = DoGetBackendComplete(rv);
        break;
      case OPEN:
        rv = DoOpen();
        break;
      case OPEN_COMPLETE:
        rv = DoOpenComplete(rv);
        break;
      case READ:
        rv = DoRead();
        break;
      case READ_COMPLETE:
        rv = DoReadComplete(rv);
        break;
      case WAIT_FOR_DATA_READY_DONE:
        rv = DoWaitForDataReadyDone();
        break;
      case CREATE_OR_OPEN:
        rv = DoCreateOrOpen();
        break;
      case CREATE_OR_OPEN_COMPLETE:
        rv = DoCreateOrOpenComplete(rv);
        break;
      case WRITE:
        rv = DoWrite();
        break;
      case WRITE_COMPLETE:
        rv = DoWriteComplete(rv);
        break;
      case SET_DONE:
        rv = DoSetDone();
        break;
      default:
        rv = OK;
        NOTREACHED();
    }
  } while (rv != ERR_IO_PENDING && state_ != NONE);

  return rv;
}

int CacheBasedQuicServerInfo::DoGetBackendComplete(int rv) {
  if (rv == OK) {
    backend_ = data_shim_->backend;
    state_ = OPEN;
  } else {
    RecordQuicServerInfoFailure(GET_BACKEND_FAILURE);
    state_ = WAIT_FOR_DATA_READY_DONE;
  }
  return OK;
}

int CacheBasedQuicServerInfo::DoOpenComplete(int rv) {
  if (rv == OK) {
    entry_ = data_shim_->entry;
    state_ = READ;
    found_entry_ = true;
  } else {
    RecordQuicServerInfoFailure(OPEN_FAILURE);
    state_ = WAIT_FOR_DATA_READY_DONE;
  }

  return OK;
}

int CacheBasedQuicServerInfo::DoReadComplete(int rv) {
  if (rv > 0)
    data_.assign(read_buffer_->data(), rv);
  else if (rv < 0)
    RecordQuicServerInfoFailure(READ_FAILURE);

  state_ = WAIT_FOR_DATA_READY_DONE;
  return OK;
}

int CacheBasedQuicServerInfo::DoWriteComplete(int rv) {
  if (rv < 0)
    RecordQuicServerInfoFailure(WRITE_FAILURE);
  state_ = SET_DONE;
  return OK;
}

int CacheBasedQuicServerInfo::DoCreateOrOpenComplete(int rv) {
  if (rv != OK) {
    RecordQuicServerInfoFailure(CREATE_OR_OPEN_FAILURE);
    state_ = SET_DONE;
  } else {
    if (!entry_) {
      entry_ = data_shim_->entry;
      found_entry_ = true;
    }
    DCHECK(entry_);
    state_ = WRITE;
  }
  return OK;
}

int CacheBasedQuicServerInfo::DoGetBackend() {
  state_ = GET_BACKEND_COMPLETE;
  return backend_cache_->GetBackend(&data_shim_->backend, io_callback_);
}

int CacheBasedQuicServerInfo::DoOpen() {
  state_ = OPEN_COMPLETE;
  return backend_->OpenEntry(key(), &data_shim_->entry, io_callback_);
}

int CacheBasedQuicServerInfo::DoRead() {
  const int size = entry_->GetDataSize(0 /* index */);
  if (!size) {
    state_ = WAIT_FOR_DATA_READY_DONE;
    return OK;
  }

  read_buffer_ = new IOBuffer(size);
  state_ = READ_COMPLETE;
  return entry_->ReadData(
      0 /* index */, 0 /* offset */, read_buffer_.get(), size, io_callback_);
}

int CacheBasedQuicServerInfo::DoWrite() {
  write_buffer_ = new IOBuffer(new_data_.size());
  memcpy(write_buffer_->data(), new_data_.data(), new_data_.size());
  state_ = WRITE_COMPLETE;

  return entry_->WriteData(0 /* index */,
                           0 /* offset */,
                           write_buffer_.get(),
                           new_data_.size(),
                           io_callback_,
                           true /* truncate */);
}

int CacheBasedQuicServerInfo::DoCreateOrOpen() {
  state_ = CREATE_OR_OPEN_COMPLETE;
  if (entry_)
    return OK;

  if (found_entry_) {
    return backend_->OpenEntry(key(), &data_shim_->entry, io_callback_);
  }

  return backend_->CreateEntry(key(), &data_shim_->entry, io_callback_);
}

int CacheBasedQuicServerInfo::DoWaitForDataReadyDone() {
  DCHECK(!ready_);
  state_ = NONE;
  ready_ = true;
  // We close the entry because, if we shutdown before ::Persist is called,
  // then we might leak a cache reference, which causes a DCHECK on shutdown.
  if (entry_)
    entry_->Close();
  entry_ = NULL;

  RecordQuicServerInfoStatus(QUIC_SERVER_INFO_PARSE);
  if (!Parse(data_)) {
    if (data_.empty())
      RecordQuicServerInfoFailure(PARSE_NO_DATA_FAILURE);
    else
      RecordQuicServerInfoFailure(PARSE_FAILURE);
  }

  UMA_HISTOGRAM_TIMES("Net.QuicServerInfo.DiskCacheLoadTime",
                      base::TimeTicks::Now() - load_start_time_);
  return OK;
}

int CacheBasedQuicServerInfo::DoSetDone() {
  if (entry_)
    entry_->Close();
  entry_ = NULL;
  new_data_.clear();
  state_ = NONE;
  return OK;
}

void CacheBasedQuicServerInfo::RecordQuicServerInfoStatus(
    QuicServerInfoAPICall call) {
  if (!backend_) {
    UMA_HISTOGRAM_ENUMERATION("Net.QuicDiskCache.APICall.NoBackend", call,
                              QUIC_SERVER_INFO_NUM_OF_API_CALLS);
  } else if (backend_->GetCacheType() == MEMORY_CACHE) {
    UMA_HISTOGRAM_ENUMERATION("Net.QuicDiskCache.APICall.MemoryCache", call,
                              QUIC_SERVER_INFO_NUM_OF_API_CALLS);
  } else {
    UMA_HISTOGRAM_ENUMERATION("Net.QuicDiskCache.APICall.DiskCache", call,
                              QUIC_SERVER_INFO_NUM_OF_API_CALLS);
  }
}

void CacheBasedQuicServerInfo::RecordLastFailure() {
  if (last_failure_ != NO_FAILURE) {
    UMA_HISTOGRAM_ENUMERATION(
        "Net.QuicDiskCache.FailureReason.WaitForDataReady",
        last_failure_, NUM_OF_FAILURES);
  }
  last_failure_ = NO_FAILURE;
}

void CacheBasedQuicServerInfo::RecordQuicServerInfoFailure(
    FailureReason failure) {
  last_failure_ = failure;

  if (!backend_) {
    UMA_HISTOGRAM_ENUMERATION("Net.QuicDiskCache.FailureReason.NoBackend",
                              failure, NUM_OF_FAILURES);
  } else if (backend_->GetCacheType() == MEMORY_CACHE) {
    UMA_HISTOGRAM_ENUMERATION("Net.QuicDiskCache.FailureReason.MemoryCache",
                              failure, NUM_OF_FAILURES);
  } else {
    UMA_HISTOGRAM_ENUMERATION("Net.QuicDiskCache.FailureReason.DiskCache",
                              failure, NUM_OF_FAILURES);
  }
}

}  // namespace net