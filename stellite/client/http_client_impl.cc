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

#include "stellite/client/http_client_impl.h"

#include "base/memory/ptr_util.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_info.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_response_info.h"
#include "net/url_request/url_fetcher.h"
#include "stellite/include/http_client.h"
#include "stellite/include/http_request.h"
#include "stellite/include/http_response.h"
#include "url/gurl.h"

const int kDefaultRequestTimeout = 60 * 1000; // 30 seconds
const char* kTimeoutMessage = "timeout error";

namespace stellite {

HttpClientImpl::HttpClientImpl(HttpFetcher* http_fetcher,
                               HttpResponseDelegate* response_delegate)
    : http_fetcher_(http_fetcher),
      response_delegate_(response_delegate),
      weak_factory_(this) {
  CHECK(response_delegate_);
}

HttpClientImpl::~HttpClientImpl() {
  TearDown();
}

int HttpClientImpl::Request(const HttpRequest& request) {
  return Request(request, kDefaultRequestTimeout);
}

int HttpClientImpl::Request(const HttpRequest& http_request, int timeout) {
  return http_fetcher_->Request(http_request, timeout,
                                weak_factory_.GetWeakPtr());
}

bool HttpClientImpl::AppendChunkToUpload(int request_id,
                                         const std::string& content,
                                         bool is_last) {
  return http_fetcher_->AppendChunkToUpload(request_id, content, is_last);
}

void HttpClientImpl::OnTaskComplete(int request_id,
                                    const net::URLFetcher* source,
                                    const net::HttpResponseInfo* response) {

  HttpResponse* http_response = FindResponse(request_id);
  if (!http_response) {
    http_response = NewResponse(request_id, source, response);
  }

  std::string payload;
  source->GetResponseAsString(&payload);
  DCHECK(response_delegate_);
  response_delegate_->OnHttpResponse(request_id, *http_response,
                                     payload.data(), payload.size());

  ReleaseResponse(request_id);
}

void HttpClientImpl::OnTaskHeader(int request_id,
                                  const net::URLFetcher* source,
                                  const net::HttpResponseInfo* response_info) {
  NewResponse(request_id, source, response_info);
}

void HttpClientImpl::OnTaskStream(int request_id,
                                  const char* data, size_t len, bool fin) {
  HttpResponse* http_response = FindResponse(request_id);
  DCHECK(http_response);

  DCHECK(response_delegate_);
  response_delegate_->OnHttpStream(request_id, *http_response, data, len, fin);

  if (fin) {
    ReleaseResponse(request_id);
  }
}

void HttpClientImpl::OnTaskError(int request_id,
                                 const net::URLFetcher* source,
                                 int error_code) {
  response_delegate_->OnHttpError(request_id, error_code,
                                  net::ErrorToString(error_code));
}

HttpResponse* HttpClientImpl::FindResponse(int request_id) {
  HttpResponseMap::iterator it = response_map_.find(request_id);
  return it == response_map_.end() ? nullptr : it->second.get();
}

HttpResponse* HttpClientImpl::NewResponse(
    int request_id, const net::URLFetcher* source,
    const net::HttpResponseInfo* response_info) {
  if (source == nullptr) {
    return nullptr;
  }

  HttpResponse* http_response = new HttpResponse();

  // url
  http_response->url = source->GetURL().spec();

  scoped_refptr<net::HttpResponseHeaders> headers =
      source->GetResponseHeaders();
  if (headers.get()) {

    // headers
    http_response->headers.Reset(headers->raw_headers());

    // status text
    http_response->status_text = headers->GetStatusText();

    // mime type
    headers->GetMimeType(&http_response->mime_type);

    // charset
    headers->GetCharset(&http_response->charset);

    // response code
    http_response->response_code = headers->response_code();
  }

  if (response_info) {
    http_response->connection_info =
        static_cast<HttpResponse::ConnectionInfo>(
            response_info->connection_info);

    http_response->connection_info_desc =
        net::HttpResponseInfo::ConnectionInfoToString(
            response_info->connection_info);

    http_response->request_time = response_info->request_time.ToTimeT();
    http_response->response_time = response_info->request_time.ToTimeT();

    http_response->was_alpn_negotiated = response_info->was_alpn_negotiated;
    http_response->network_accessed = response_info->network_accessed;
    http_response->server_data_unavailable =
        response_info->server_data_unavailable;
    http_response->was_cached = response_info->was_cached;
    http_response->was_fetched_via_proxy = response_info->was_fetched_via_proxy;
    http_response->was_fetched_via_spdy = response_info->was_fetched_via_spdy;
  }

  response_map_.insert(
      std::make_pair(request_id, base::WrapUnique(http_response)));

  return http_response;
}

void HttpClientImpl::ReleaseResponse(int request_id) {
  HttpResponseMap::iterator it = response_map_.find(request_id);
  if (it == response_map_.end()) {
    return;
  }
  response_map_.erase(it);
}

void HttpClientImpl::TearDown() {}

} // namespace stellite
