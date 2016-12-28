// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stellite/fetcher/http_fetcher_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/sequenced_task_runner.h"
#include "net/base/upload_data_stream.h"
#include "net/url_request/url_fetcher_factory.h"
#include "net/url_request/url_fetcher_response_writer.h"
#include "stellite/fetcher/http_fetcher_core.h"
#include "stellite/fetcher/http_fetcher_delegate.h"

namespace stellite {

static URLFetcherFactory* g_factory = NULL;

HttpFetcherImpl::HttpFetcherImpl(const GURL& url,
                                 RequestType request_type,
                                 HttpFetcherDelegate* d,
                                 bool stream_response)
    : core_(new net::HttpFetcherCore(this, url, request_type, d,
                                     stream_response)) {
}

HttpFetcherImpl::~HttpFetcherImpl() {
  core_->Stop();
}

void HttpFetcherImpl::SetUploadData(const std::string& upload_content_type,
                                    const std::string& upload_content) {
  core_->SetUploadData(upload_content_type, upload_content);
}

void HttpFetcherImpl::SetUploadFilePath(
    const std::string& upload_content_type,
    const base::FilePath& file_path,
    uint64_t range_offset,
    uint64_t range_length,
    scoped_refptr<base::TaskRunner> file_task_runner) {
  core_->SetUploadFilePath(upload_content_type,
                           file_path,
                           range_offset,
                           range_length,
                           file_task_runner);
}

void HttpFetcherImpl::SetUploadStreamFactory(
    const std::string& upload_content_type,
    const CreateUploadStreamCallback& callback) {
  core_->SetUploadStreamFactory(upload_content_type, callback);
}

void HttpFetcherImpl::SetChunkedUpload(const std::string& content_type) {
  core_->SetChunkedUpload(content_type);
}

void HttpFetcherImpl::AppendChunkToUpload(const std::string& data,
                                         bool is_last_chunk) {
  DCHECK(data.length());
  core_->AppendChunkToUpload(data, is_last_chunk);
}

void HttpFetcherImpl::SetReferrer(const std::string& referrer) {
  core_->SetReferrer(referrer);
}

void HttpFetcherImpl::SetReferrerPolicy(
    net::URLRequest::ReferrerPolicy referrer_policy) {
  core_->SetReferrerPolicy(referrer_policy);
}

void HttpFetcherImpl::SetLoadFlags(int load_flags) {
  core_->SetLoadFlags(load_flags);
}

int HttpFetcherImpl::GetLoadFlags() const {
  return core_->GetLoadFlags();
}

void HttpFetcherImpl::SetExtraRequestHeaders(
    const std::string& extra_request_headers) {
  core_->SetExtraRequestHeaders(extra_request_headers);
}

void HttpFetcherImpl::AddExtraRequestHeader(const std::string& header_line) {
  core_->AddExtraRequestHeader(header_line);
}

void HttpFetcherImpl::SetRequestContext(
    net::URLRequestContextGetter* request_context_getter) {
  core_->SetRequestContext(request_context_getter);
}

void HttpFetcherImpl::SetInitiatorURL(const GURL& initiator) {
  core_->SetInitiatorURL(initiator);
}

void HttpFetcherImpl::SetURLRequestUserData(
    const void* key,
    const CreateDataCallback& create_data_callback) {
  core_->SetURLRequestUserData(key, create_data_callback);
}

void HttpFetcherImpl::SetStopOnRedirect(bool stop_on_redirect) {
  core_->SetStopOnRedirect(stop_on_redirect);
}

void HttpFetcherImpl::SetAutomaticallyRetryOn5xx(bool retry) {
  core_->SetAutomaticallyRetryOn5xx(retry);
}

void HttpFetcherImpl::SetMaxRetriesOn5xx(int max_retries) {
  core_->SetMaxRetriesOn5xx(max_retries);
}

int HttpFetcherImpl::GetMaxRetriesOn5xx() const {
  return core_->GetMaxRetriesOn5xx();
}

base::TimeDelta HttpFetcherImpl::GetBackoffDelay() const {
  return core_->GetBackoffDelay();
}

void HttpFetcherImpl::SetAutomaticallyRetryOnNetworkChanges(int max_retries) {
  core_->SetAutomaticallyRetryOnNetworkChanges(max_retries);
}

void HttpFetcherImpl::SaveResponseToFileAtPath(
    const base::FilePath& file_path,
    scoped_refptr<base::SequencedTaskRunner> file_task_runner) {
  core_->SaveResponseToFileAtPath(file_path, file_task_runner);
}

void HttpFetcherImpl::SaveResponseToTemporaryFile(
    scoped_refptr<base::SequencedTaskRunner> file_task_runner) {
  core_->SaveResponseToTemporaryFile(file_task_runner);
}

void HttpFetcherImpl::SaveResponseWithWriter(
    std::unique_ptr<net::URLFetcherResponseWriter> response_writer) {
  core_->SaveResponseWithWriter(std::move(response_writer));
}

net::HttpResponseHeaders* HttpFetcherImpl::GetResponseHeaders() const {
  return core_->GetResponseHeaders();
}

net::HostPortPair HttpFetcherImpl::GetSocketAddress() const {
  return core_->GetSocketAddress();
}

bool HttpFetcherImpl::WasFetchedViaProxy() const {
  return core_->WasFetchedViaProxy();
}

bool HttpFetcherImpl::WasCached() const {
  return core_->WasCached();
}

int64_t HttpFetcherImpl::GetReceivedResponseContentLength() const {
  return core_->GetReceivedResponseContentLength();
}

int64_t HttpFetcherImpl::GetTotalReceivedBytes() const {
  return core_->GetTotalReceivedBytes();
}

void HttpFetcherImpl::Start() {
  core_->Start();
}

void HttpFetcherImpl::Stop() {
  core_->Stop();
}

const GURL& HttpFetcherImpl::GetOriginalURL() const {
  return core_->GetOriginalURL();
}

const GURL& HttpFetcherImpl::GetURL() const {
  return core_->GetURL();
}

const net::URLRequestStatus& HttpFetcherImpl::GetStatus() const {
  return core_->GetStatus();
}

int HttpFetcherImpl::GetResponseCode() const {
  return core_->GetResponseCode();
}

void HttpFetcherImpl::ReceivedContentWasMalformed() {
  core_->ReceivedContentWasMalformed();
}

bool HttpFetcherImpl::GetResponseAsString(
    std::string* out_response_string) const {
  return core_->GetResponseAsString(out_response_string);
}

bool HttpFetcherImpl::GetResponseAsFilePath(
    bool take_ownership,
    base::FilePath* out_response_path) const {
  return core_->GetResponseAsFilePath(take_ownership, out_response_path);
}

// static
void HttpFetcherImpl::CancelAll() {
  net::HttpFetcherCore::CancelAll();
}

// static
void HttpFetcherImpl::SetIgnoreCertificateRequests(bool ignored) {
  net::HttpFetcherCore::SetIgnoreCertificateRequests(ignored);
}

// static
int HttpFetcherImpl::GetNumFetcherCores() {
  return net::HttpFetcherCore::GetNumFetcherCores();
}

HttpFetcherDelegate* HttpFetcherImpl::delegate() const {
  return core_->delegate();
}

// static
URLFetcherFactory* HttpFetcherImpl::factory() {
  return g_factory;
}

// static
void HttpFetcherImpl::set_factory(URLFetcherFactory* factory) {
  g_factory = factory;
}

}  // namespace net
