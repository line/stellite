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

#include "stellite/client/network_transaction_consumer.h"

#include "base/time/time.h"
#include "stellite/client/network_transaction_factory.h"
#include "net/base/elements_upload_data_stream.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/http/http_transaction.h"
#include "net/log/net_log.h"

const size_t kBufferSize = 4096;

namespace net {

NetworkTransactionConsumer::NetworkTransactionConsumer(
    const HttpRequestInfo& request,
    const std::stringstream& request_body,
    RequestPriority priority,
    NetworkTransactionFactory* factory,
    bool stream_response,
    int timeout)
    : request_(request),
      request_body_(request_body.str()),
      priority_(priority),
      transaction_factory_(factory),
      state_(IDLE),
      visitor_(nullptr),
      stream_response_(stream_response),
      timeout_(timeout) {
  CHECK(transaction_factory_);
}

NetworkTransactionConsumer::~NetworkTransactionConsumer() {
  TearDown();
}

void NetworkTransactionConsumer::Start() {
  DCHECK(state_ == IDLE);
  if (visitor_ == nullptr) {
    LOG(ERROR) << "NetworkTransactionConsumer visitor is nullptr";
    return;
  }

  state_ = STARTING;

  // Setting upload request body
  if (request_body_.size()) {
    std::vector<std::unique_ptr<UploadElementReader>> element_readers;
    element_readers.push_back(
        std::unique_ptr<UploadBytesElementReader>(
            new UploadBytesElementReader(request_body_.c_str(),
                                         request_body_.size())));
    upload_data_stream_.reset(
        new ElementsUploadDataStream(std::move(element_readers), 0));
    request_.upload_data_stream = upload_data_stream_.get();
  }

  int result = transaction_factory_->CreateTransaction(priority_,
                                                       &http_transaction_);
  if (result != OK) {
    if (stream_response_) {
      visitor_->OnTransactionStream(this, NULL, 0, result);
    } else {
      visitor_->OnTransactionTerminate(this, result);
    }
    return;
  }

  result = http_transaction_->Start(
      &request_,
      base::Bind(&NetworkTransactionConsumer::OnIOComplete, this),
      BoundNetLog());

  DCHECK(transaction_waiting_since_.is_null());

  SetTransactionTimeout(timeout_);

  if (result == ERR_IO_PENDING) {
    return;
  }

  DidStart(result);
}

void NetworkTransactionConsumer::TearDown() {
  if (http_transaction_.get()) {
    http_transaction_->DoneReading();
  }
  http_transaction_.reset();

  upload_data_stream_.reset();
  request_body_.clear();

  read_buf_ = nullptr;
  contents_ = nullptr;
}

void NetworkTransactionConsumer::DidStart(int result) {
  if (result != OK) {
    DidFinish(result);
  } else {
    Read();
  }
}

void NetworkTransactionConsumer::DidRead(int result) {
  if (result <= 0) {
    DidFinish(result);
    return;
  }

  if (visitor_ && stream_response_) {
    visitor_->OnTransactionStream(this, read_buf_->data(), result, result);
  }

  if (!stream_response_) {
    while (contents_->RemainingCapacity() < result) {
      contents_->SetCapacity(contents_->capacity() * 2);
    }

    memcpy(contents_->data(), read_buf_->data(), result);
    contents_->set_offset(contents_->offset() + result);
  }

  Read();
}

void NetworkTransactionConsumer::DidFinish(int result) {
  state_ = DONE;
  error_ = result;

  DCHECK(transaction_timer_.get());
  DCHECK(transaction_timer_->IsRunning());
  transaction_timer_->Stop();

  // Delete |this|
  if (!visitor_) {
    return;
  }

  if (stream_response_) {
    visitor_->OnTransactionStream(this, NULL, 0, OK);
  } else {
    visitor_->OnTransactionTerminate(this, result);
  }
}

void NetworkTransactionConsumer::Read() {
  state_ = READING;

  if (!read_buf_.get()) {
    read_buf_ = new IOBuffer(kBufferSize);
  }

  if (!contents_.get() && !stream_response_) {
    contents_ = new GrowableIOBuffer();
    contents_->SetCapacity(kBufferSize);
  }

  int result = http_transaction_->Read(
      read_buf_.get(),
      kBufferSize,
      base::Bind(&NetworkTransactionConsumer::OnIOComplete, this));

  if (result == net::ERR_IO_PENDING) {
    SetTransactionTimeout(timeout_);
    return;
  }

  DidRead(result);
}

void NetworkTransactionConsumer::OnIOComplete(int result) {
  switch (state_) {
    case STARTING:
      DidStart(result);
      break;
    case READING:
      DidRead(result);
      break;
    default:
      LOG(ERROR) << "Unreachable state:" << state_;
  }
}

const HttpResponseInfo* NetworkTransactionConsumer::response_info() const {
  if (!http_transaction_.get()) {
    return NULL;
  }
  return http_transaction_->GetResponseInfo();
}

void NetworkTransactionConsumer::SetTransactionTimeout(
    int timeout_milliseconds) {

  if (!transaction_timer_.get()) {
    transaction_timer_.reset(new base::OneShotTimer());
  }

  // Aborting HTTP transaction
  if (transaction_timer_->IsRunning()) {
    transaction_timer_->Stop();
  }

  // Memorize a starting time
  transaction_waiting_since_ = base::TimeTicks::Now();

  transaction_timer_->Start(
      FROM_HERE,
      base::TimeDelta::FromMilliseconds(timeout_milliseconds),
      this,
      &NetworkTransactionConsumer::OnTransactionTimeout);
}

void NetworkTransactionConsumer::OnTransactionTimeout() {
  // Cancel transaction
  TearDown();

  if (!visitor_) {
    LOG(ERROR) << "Visitor is not set";
    return;
  }

  // It is not determination about connection timeout or request timeout
  // but we decide the state is |STARTING| in the context of connection timeout.
  if (state_ == STARTING) {
    visitor_->OnTransactionTerminate(this, ERR_CONNECTION_TIMED_OUT);
  }

  if (state_ == READING) {
    visitor_->OnTransactionTerminate(this, ERR_TIMED_OUT);
  }
}

} // namespace net
