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

#include "stellite/server/proxy_stream.h"

#include "base/strings/string_number_conversions.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/spdy/spdy_http_utils.h"
#include "stellite/fetcher/http_rewrite.h"
#include "stellite/fetcher/spdy_utils.h"
#include "stellite/logging/logging.h"
#include "stellite/server/parse_util.h"
#include "stellite/server/server_session.h"
#include "stellite/stats/server_stats_macro.h"

namespace net {

const char* kHttp11 = "HTTP/1.1";
const char* kHttpContentLength = "Content-Length:";
const char* kLocation = "Location:";
const char* kReturn = "\n";
const SpdyMajorVersion kProtocolVersion = HTTP2;

ProxyStream::ProxyStream(const ServerConfig& server_config,
                         QuicStreamId id, ServerSession* session,
                         HttpFetcher* http_fetcher,
                         const HttpRewrite* http_rewrite)
    : QuicSimpleServerStream(id, session),
      per_connection_session_(session),
      http_fetcher_(http_fetcher),
      http_rewrite_(http_rewrite),
      proxy_timeout_(server_config.proxy_timeout() * 1000),
      proxy_pass_(server_config.proxy_pass()),
      weak_factory_(this) {
}

ProxyStream::~ProxyStream() {
}

void ProxyStream::OnFetchComplete(const URLFetcher* source, int64_t msec) {
  if (source == NULL) {
    FLOG(ERROR) << "URLFetcher reseponse callback with NULL fetcher.";
    return;
  }

  // Headers
  scoped_refptr<HttpResponseHeaders> headers = source->GetResponseHeaders();
  if (headers == nullptr) {
    headers = BuildCustomHeader(source->GetResponseCode(),
                                source->GetURL(), "");
  }

  SpdyHeaderBlock spdy_headers;
  CreateSpdyHeadersFromHttpResponse(*headers, &spdy_headers);

  // Body
  std::string body;
  source->GetResponseAsString(&body);

  SendHeadersAndBody(std::move(spdy_headers), body);

  per_connection_session_->AddHttpStat(kHttpReceived, 1);

  WriteAccessLog(source->GetResponseCode(), msec);
}

void ProxyStream::OnFetchTimeout(int64_t msec) {
  ErrorResponse(408, "Request Timeout", msec);
  per_connection_session_->AddHttpStat(kHttpTimeout, 1);
}

void ProxyStream::SendResponse() {
  SpdyHeaderBlock* spdy_headers = request_headers();
  DCHECK(spdy_headers);

  // TODO(@snibug): Check URL validation
  std::string url = SpdyUtils::GetUrlFromHeaderBlock(*spdy_headers);
  proxy_url_ = GURL(url);

  // Check URL rewrite rule
  std::string rewrited_url;
  if (http_rewrite_ && http_rewrite_->Rewrite(proxy_url_.spec(),
                                              &rewrited_url)) {
    proxy_url_ = GURL(rewrited_url);
  }

  // Proxy pass
  if (!proxy_pass_.is_empty()) {
    const int origin_length = proxy_url_.GetOrigin().spec().size();
    std::string uri = proxy_url_.spec().substr(origin_length);
    proxy_url_ = GURL(proxy_pass_.spec() + uri);
  }

  // HTTP method
  proxy_method_ = ParseMethod(*spdy_headers, kProtocolVersion);

  // Request header
  HttpRequestHeaders http_request_headers;
  ConvertSpdyHeaderToHttpRequest(*spdy_headers,
                                 kProtocolVersion,
                                 &http_request_headers);

  // Keep |Host| of the request
  http_request_headers.SetHeader("Host", proxy_url_.host());

  // Keep the real IP address of the request
  const IPEndPoint& client_address =
      per_connection_session_->connection()->peer_address();
  std::string client_ip(client_address.ToStringWithoutPort());
  http_request_headers.SetHeader("X-Real-IP", client_ip);
  http_request_headers.SetHeader("X-Forwarded-For", client_ip);

  http_fetcher_->Fetch(proxy_url_, proxy_method_, http_request_headers, body(),
                       proxy_timeout_, weak_factory_.GetWeakPtr());

  per_connection_session_->AddHttpStat(kHttpSent, 1);
}

void ProxyStream::ErrorResponse(int error_code,
                                const std::string& error_message,
                                int64_t msec) {
  scoped_refptr<HttpResponseHeaders> headers
      = BuildCustomHeader(error_code, GURL(), error_message);

  SpdyHeaderBlock spdy_headers;
  CreateSpdyHeadersFromHttpResponse(*headers, &spdy_headers);

  SendHeadersAndBody(std::move(spdy_headers), error_message);

  WriteAccessLog(error_code, msec);
}

scoped_refptr<HttpResponseHeaders> ProxyStream::BuildCustomHeader(
    const int status_code, const GURL& url, const std::string& contents) {
  std::ostringstream header_stream;
  header_stream << kHttp11;
  header_stream << " ";
  header_stream << status_code;
  header_stream << kReturn;

  if (status_code == HTTP_MOVED_PERMANENTLY || status_code == HTTP_FOUND) {
    header_stream << kLocation;
    header_stream << " ";
    header_stream << url.spec();
    header_stream << kReturn;
  }

  header_stream << kHttpContentLength;
  header_stream << " ";
  header_stream << contents.size();
  header_stream << kReturn;

  std::string raw_headers(header_stream.str());
  HeadersToRaw(&raw_headers);
  return new HttpResponseHeaders(raw_headers);
}

void ProxyStream::WriteAccessLog(int response_code, int64_t msec) {
  std::string request_method(
      proxy_method_ == URLFetcher::GET ? "GET" :
      proxy_method_ == URLFetcher::POST ? "POST" :
      proxy_method_ == URLFetcher::HEAD ? "HEAD" :
      proxy_method_ == URLFetcher::DELETE_REQUEST ? "DELETE" :
      proxy_method_ == URLFetcher::PUT ? "PUT" :
      proxy_method_ == URLFetcher::PATCH ? "PATCH" : "UNKNOWN");

  const IPEndPoint& client_address =
      per_connection_session_->connection()->peer_address();
  FLOG(ACCESS) << client_address.ToStringWithoutPort()
               << "::" << msec
               << "::" << response_code
               << "::" << request_method
               << "::" << proxy_url_;
}

}  // namespace net
