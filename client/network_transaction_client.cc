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

#include "stellite/client/network_transaction_client.h"

#include "base/single_thread_task_runner.h"
#include "stellite/client/contents_filter_context.h"
#include "stellite/client/network_transaction_consumer.h"
#include "stellite/client/network_transaction_factory.h"
#include "stellite/include/http_client.h"
#include "stellite/include/http_request.h"
#include "stellite/include/http_response.h"
#include "net/cert/cert_verifier.h"
#include "net/filter/filter.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_server_properties_impl.h"
#include "net/proxy/proxy_service.h"
#include "net/socket/next_proto.h"
#include "net/ssl/ssl_config_service_defaults.h"

namespace net {

const int kBufferSize = 4096;

NetworkTransactionClient::NetworkTransactionClient(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    NetworkTransactionFactory* transaction_factory,
    size_t active_request_capacity)
    : task_runner_(task_runner),
      transaction_factory_(transaction_factory),
      active_request_capacity_(active_request_capacity),
      last_request_id_(1) {
  CHECK(active_request_capacity > 0);
}

NetworkTransactionClient::~NetworkTransactionClient() {
  TearDown();
}

int NetworkTransactionClient::Request(
    const stellite::HttpRequest& request,
    stellite::HttpResponseDelegate* response_delegate,
    const RequestPriority priority,
    bool stream_response,
    int timeout) {
  if (!response_delegate) {
    LOG(ERROR) << "Response delegate is null";
    return 0;
  }

  HttpRequestInfo request_info;
  RewriteHttpRequest(&request_info, &request);

  scoped_refptr<NetworkTransactionConsumer> transaction =
      new NetworkTransactionConsumer(
          request_info, request.upload_stream, priority, transaction_factory_,
          stream_response, timeout);

  int request_id = ++last_request_id_;
  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&NetworkTransactionClient::OnTransactionBegin,
                 this,
                 request_id,
                 transaction,
                 response_delegate));
  return request_id;
}

void NetworkTransactionClient::DeleteTransaction(
    scoped_refptr<NetworkTransactionConsumer> transaction) {
  CHECK(base::MessageLoop::current()->task_runner() == task_runner_);

  CHECK(transaction);
  response_delegate_map_.erase(transaction.get());

  active_entries_.remove(transaction);
  transaction->TearDown();

  if (pending_entries_.empty()) {
    return;
  }

  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&NetworkTransactionClient::OnPendingTransactionBegin, this));
}

void NetworkTransactionClient::OnTransactionTerminate(
    scoped_refptr<NetworkTransactionConsumer> transaction, int result) {
  CHECK(base::MessageLoop::current()->task_runner() == task_runner_);
  CHECK(transaction);

  // Response delegate
  ResponseDelegateMap::const_iterator it =
      response_delegate_map_.find(transaction.get());
  if (it == response_delegate_map_.end()) {
    LOG(ERROR) << "Terminate: response delegate is not visible";
    return;
  }

  int request_id = it->second.first;
  stellite::HttpResponseDelegate* response_delegate = it->second.second;
  if (!response_delegate) {
    LOG(ERROR) << "Response delegate is null";
    return;
  }

  // Remove the transaction and release delegate after response callback
  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&NetworkTransactionClient::DeleteTransaction,
                 this,
                 transaction));

  if (result < 0 && result != OK) {
    response_delegate->OnHttpError(request_id, result,
                                   ErrorToShortString(result));
    return;
  }

  stellite::HttpResponse http_response;
  const HttpResponseInfo* response_info = transaction->response_info();
  if (!RewriteHttpResponse(&http_response, response_info)) {
    response_delegate->OnHttpError(request_id, -1, "fail rewrite response");
    return;
  }

  GrowableIOBuffer* buffer = transaction->buffer();

  size_t iter = 0;
  std::string encoding;
  if (!response_info->headers->EnumerateHeader(&iter, "content-encoding",
                                               &encoding)) {
    response_delegate->OnHttpResponse(request_id, http_response,
        buffer ? buffer->StartOfBuffer() : nullptr,
        buffer ? buffer->offset() : 0);
    return;
  }

  Filter::FilterType filter_type = Filter::ConvertEncodingToType(encoding);
  if (filter_type == Filter::FilterType::FILTER_TYPE_UNSUPPORTED) {
    LOG(ERROR) << "Unsupported content-encoding type";
    response_delegate->OnHttpResponse(
        request_id,
        http_response,
        buffer ? buffer->StartOfBuffer() : nullptr,
        buffer ? buffer->offset() : 0);
    return;
  }

  std::unique_ptr<std::vector<char>> out;
  if (!Decode(filter_type, buffer->StartOfBuffer(), buffer->offset(), &out)) {
    LOG(ERROR) << "Failed to decode the encoded body";
    response_delegate->OnHttpResponse(request_id, http_response,
        buffer ? buffer->StartOfBuffer() : nullptr,
        buffer ? buffer->offset() : 0);
    return;
  }

  response_delegate->OnHttpResponse(request_id, http_response, &out->front(),
                                    out->size());
}

void NetworkTransactionClient::OnTransactionStream(
    scoped_refptr<NetworkTransactionConsumer> transaction,
    const char* stream_data, size_t len, int result) {
  // Removes the transaction after calling a delegate
  if (len == 0 || (result < 0 && result != OK)) {
    task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&NetworkTransactionClient::DeleteTransaction,
                   this,
                   transaction));
  }

  // Response delegate
  ResponseDelegateMap::const_iterator it = response_delegate_map_.find(
      transaction.get());
  if (it == response_delegate_map_.end()) {
    LOG(ERROR) << "On stream: response delegate is not visible";
    return;
  }

  int request_id = it->second.first;
  stellite::HttpResponseDelegate* response_delegate = it->second.second;
  if (!response_delegate) {
    LOG(ERROR) << "Response delegate is null";
    return;
  }

  if (result < 0 && result != OK) {
    response_delegate->OnHttpError(request_id, result,
                                   ErrorToShortString(result));
    return;
  }

  const HttpResponseInfo* response_info = transaction->response_info();
  stellite::HttpResponse http_response;
  if (!RewriteHttpResponse(&http_response, response_info)) {
    response_delegate->OnHttpError(request_id, -1,
                                   "fail to rewrite http response");
    return;
  }

  response_delegate->OnHttpStream(request_id, http_response, stream_data, len);
}

void NetworkTransactionClient::OnTransactionBegin(
    int request_id,
    scoped_refptr<NetworkTransactionConsumer> transaction,
    stellite::HttpResponseDelegate* delegate) {
  CHECK(base::MessageLoop::current()->task_runner() == task_runner_);
  response_delegate_map_.insert(
      std::make_pair(transaction.get(),
                     std::make_pair(request_id, delegate)));

  CHECK(transaction);
  transaction->set_visitor(this);

  if (active_entries_.size() < active_request_capacity_) {
    active_entries_.push_back(transaction);
    transaction->Start();
    return;
  }

  pending_entries_.push(transaction);
}

void NetworkTransactionClient::OnPendingTransactionBegin() {
  CHECK(base::MessageLoop::current()->task_runner() == task_runner_);

  if (pending_entries_.empty()) {
    return;
  }

  if (active_request_capacity_ <= active_entries_.size()) {
    return;
  }

  scoped_refptr<NetworkTransactionConsumer> pending_transaction =
      pending_entries_.front();
  pending_entries_.pop();

  pending_transaction->set_visitor(this);
  active_entries_.push_back(pending_transaction);
  pending_transaction->Start();
}

void NetworkTransactionClient::TearDown() {
  // Remove pending entries
  while (pending_entries_.size()) {
    pending_entries_.pop();
  }

  // Shut down active entries
  ActiveEntries::iterator it = active_entries_.begin();
  while (it != active_entries_.end()) {
    (*it)->TearDown();
    ++it;
  }
}

bool NetworkTransactionClient::RewriteHttpResponse(
    stellite::HttpResponse* dest,
    const HttpResponseInfo* src) {
  DCHECK(dest);
  DCHECK(src);

  dest->was_cached = src->was_cached;
  dest->was_fetched_via_proxy = src->was_fetched_via_proxy;
  dest->network_accessed = src->network_accessed;
  dest->was_fetched_via_spdy = src->was_fetched_via_spdy;
  dest->was_npn_negotiated = src->was_npn_negotiated;
  dest->connection_info = static_cast<stellite::HttpResponse::ConnectionInfo>(
      src->connection_info);
  dest->connection_info_desc =
      HttpResponseInfo::ConnectionInfoToString(src->connection_info);

  scoped_refptr<HttpResponseHeaders> headers = src->headers;
  if (headers == nullptr) {
    LOG(ERROR) << "HTTP header null pointer error: this is break of HTTP "
                  "or https transaction";
    return false;
  }

  dest->response_code = headers->response_code();
  dest->content_length = headers->GetContentLength();
  dest->status_text = headers->GetStatusText();

  headers->GetMimeTypeAndCharset(&dest->mime_type, &dest->charset);

  dest->headers.Reset(src->headers->raw_headers());

  return true;
}

bool NetworkTransactionClient::RewriteHttpRequest(
    HttpRequestInfo* desc,
    const stellite::HttpRequest* src) {
  if (src == nullptr || desc == nullptr) {
  LOG(ERROR) << "Invalid; failed to rewrite request";
  return false;
  }

  desc->url = GURL(src->url);
  if (!desc->url.is_valid()) {
  LOG(ERROR) << "Invalid URL specification";
  return false;
  }

  if (!src->method.size()) {
    LOG(ERROR) << "Invalid method";
    return false;
  }

  desc->method = src->method;

  desc->extra_headers.AddHeadersFromString(src->headers.ToString());

  return true;
}

bool NetworkTransactionClient::Decode(Filter::FilterType filter_type,
                                      const char* encoded, const int size,
                                      std::unique_ptr<std::vector<char>>* out) {
  ContentsFilterContext context;
  std::vector<Filter::FilterType> filter_types;
  filter_types.push_back(filter_type);
  std::unique_ptr<Filter> filter(Filter::Factory(filter_types, context));

  char decode_buffer[kBufferSize];
  const char* encode_next = encoded;
  int encode_avail_size = size;

  int code = Filter::FILTER_OK;
  std::unique_ptr<std::vector<char>> output_buffer(new std::vector<char>());
  while (code != Filter::FILTER_DONE && code != Filter::FILTER_ERROR) {
    int encoded_data_len = std::min(encode_avail_size,
                                    filter->stream_buffer_size());
    memcpy(filter->stream_buffer()->data(), encode_next, encoded_data_len);
    filter->FlushStreamBuffer(encoded_data_len);
    encode_next += encoded_data_len;
    encode_avail_size -= encoded_data_len;

    while (true) {
      int decode_data_len = kBufferSize;
      code = filter->ReadData(decode_buffer, &decode_data_len);
      if (code == Filter::FILTER_ERROR) {
        return false;
      }

      output_buffer->insert(output_buffer->end(),
                            &decode_buffer[0],
                            &decode_buffer[decode_data_len]);
      if (code == Filter::FILTER_NEED_MORE_DATA ||
          code == Filter::FILTER_DONE) {
        break;
      }
    }
  }

  out->reset(output_buffer.release());
  return true;
}

} // namespace net
