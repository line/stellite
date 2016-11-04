// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STELLITE_CLIENT_CACHE_BASED_QUIC_SERVER_INFO_H_
#define STELLITE_CLIENT_CACHE_BASED_QUIC_SERVER_INFO_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/non_thread_safe.h"
#include "base/time/time.h"
#include "net/base/completion_callback.h"
#include "net/disk_cache/disk_cache.h"
#include "net/quic/core/crypto/quic_server_info.h"
#include "stellite/include/stellite_export.h"

namespace net {

class BackendCache;
class IOBuffer;
class QuicServerId;

// CacheBasedQuicServerInfo fetches information about a QUIC server from
// our standard disk cache. Since the information is defined to be
// non-sensitive, it's OK for us to keep it on the disk.
class STELLITE_EXPORT CacheBasedQuicServerInfo
    : public QuicServerInfo,
      public NON_EXPORTED_BASE(base::NonThreadSafe) {
 public:
  CacheBasedQuicServerInfo(const QuicServerId& server_id,
                           BackendCache* backend_cache);

  // QuicServerInfo implementation.
  void Start() override;
  int WaitForDataReady(const CompletionCallback& callback) override;
  void ResetWaitForDataReadyCallback() override;
  void CancelWaitForDataReadyCallback() override;
  bool IsDataReady() override;
  bool IsReadyToPersist() override;
  void Persist() override;
  void OnExternalCacheHit() override;

 private:
  struct CacheOperationDataShim;

  enum State {
    GET_BACKEND,
    GET_BACKEND_COMPLETE,
    OPEN,
    OPEN_COMPLETE,
    READ,
    READ_COMPLETE,
    WAIT_FOR_DATA_READY_DONE,
    CREATE_OR_OPEN,
    CREATE_OR_OPEN_COMPLETE,
    WRITE,
    WRITE_COMPLETE,
    SET_DONE,
    NONE,
  };

  // Enum to track how many times data read/parse/write API calls have been
  // made to and from disk cache for QuicServerInfo.
  enum QuicServerInfoAPICall {
    QUIC_SERVER_INFO_START = 0,
    QUIC_SERVER_INFO_WAIT_FOR_DATA_READY = 1,
    QUIC_SERVER_INFO_PARSE = 2,
    QUIC_SERVER_INFO_WAIT_FOR_DATA_READY_CANCEL = 3,
    QUIC_SERVER_INFO_READY_TO_PERSIST = 4,
    QUIC_SERVER_INFO_PERSIST = 5,
    QUIC_SERVER_INFO_EXTERNAL_CACHE_HIT = 6,
    QUIC_SERVER_INFO_NUM_OF_API_CALLS = 7,
  };

  // Enum to track the reasons why reading/loading/writing QuicServerInfo to
  // and from disk cache have failed.
  enum FailureReason {
    WAIT_FOR_DATA_READY_INVALID_ARGUMENT_FAILURE = 0,
    GET_BACKEND_FAILURE = 1,
    OPEN_FAILURE = 2,
    CREATE_OR_OPEN_FAILURE = 3,
    PARSE_NO_DATA_FAILURE = 4,
    PARSE_FAILURE = 5,
    READ_FAILURE = 6,
    READY_TO_PERSIST_FAILURE = 7,
    PERSIST_NO_BACKEND_FAILURE = 8,
    WRITE_FAILURE = 9,
    NO_FAILURE = 10,
    NUM_OF_FAILURES = 11,
  };

  ~CacheBasedQuicServerInfo() override;

  // Persists |pending_write_data_| if it is not empty, otherwise serializes the
  // data and pesists it.
  void PersistInternal();

  std::string key() const;

  // The |unused| parameter is a small hack so that we can have the
  // CacheOperationDataShim object owned by the Callback that is created for
  // this method.  For details, see comment for CacheOperationDataShim above.
  void OnIOComplete(CacheOperationDataShim* unused, int rv);

  int DoLoop(int rv);

  int DoGetBackendComplete(int rv);
  int DoOpenComplete(int rv);
  int DoReadComplete(int rv);
  int DoWriteComplete(int rv);
  int DoCreateOrOpenComplete(int rv);

  int DoGetBackend();
  int DoOpen();
  int DoRead();
  int DoWrite();
  int DoCreateOrOpen();

  // DoWaitForDataReadyDone is the terminal state of the read operation.
  int DoWaitForDataReadyDone();

  // DoSetDone is the terminal state of the write operation.
  int DoSetDone();

  // Tracks in a histogram how many times data read/parse/write API calls have
  // been made to and from disk cache for QuicServerInfo.
  void RecordQuicServerInfoStatus(QuicServerInfoAPICall call);

  // Tracks in a histogram the reasons why reading/loading/writing 
  // QuicServerInfo to and from disk cache have failed. It also saves
  // the |failure| in |last_failure_|.
  void RecordQuicServerInfoFailure(FailureReason failure);

  // Tracks in a histogram if |last_failure_| is not NO_FAILURE.
  void RecordLastFailure();

  CacheOperationDataShim* data_shim_;  // Owned by |io_callback_|.
  CompletionCallback io_callback_;
  State state_;
  bool ready_;
  bool found_entry_;  // Controls the behavior of DoCreateOrOpen.
  std::string new_data_;
  std::string pending_write_data_;
  const QuicServerId server_id_;
  BackendCache* backend_cache_;
  disk_cache::Backend* backend_;
  disk_cache::Entry* entry_;
  CompletionCallback wait_for_ready_callback_;
  scoped_refptr<IOBuffer> read_buffer_;
  scoped_refptr<IOBuffer> write_buffer_;
  std::string data_;
  base::TimeTicks load_start_time_;
  FailureReason last_failure_;

  base::WeakPtrFactory<CacheBasedQuicServerInfo> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CacheBasedQuicServerInfo);
};

}  // namespace net

#endif  // STELLITE_CLIENT_CACHE_BASED_QUIC_SERVER_INFO_H_
