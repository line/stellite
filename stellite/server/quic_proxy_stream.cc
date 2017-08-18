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

#include "stellite/server/quic_proxy_stream.h"

#include "net/http/http_response_headers.h"
#include "net/quic/core/quic_session.h"
#include "net/quic/core/spdy_utils.h"
#include "net/spdy/spdy_http_utils.h"
#include "stellite/fetcher/http_fetcher.h"
#include "stellite/fetcher/spdy_utils.h"


using stellite::HttpRequest;

namespace net {

namespace  {

const char* kHeaderHost = "host";
const char* kHeaderXFF = "x-forwarded-for";
const char* kHeaderXFH = "x-forwarded-host";
const char* kHeaderTransferEncoding = "transfer-encoding";
const char* kHeaderContentLength = "content-length";
const char* kHeaderServer = "server";
const char* kBadRequest = "Bad Request";
const int64_t kBackendRequestTimeout = 60 * 1000; // 60 sec
const int kInvalidRequestId = -1;

}  // anonymous namespace

QuicProxyStream::QuicProxyStream(QuicStreamId id, QuicSpdySession* session,
                                 stellite::HttpFetcher* http_fetcher,
                                 GURL proxy_pass)
    : QuicServerStream(id, session),
      is_chunked_upload_(false),
      backend_request_id_(kInvalidRequestId),
      proxy_pass_(proxy_pass.GetOrigin()),
      http_fetcher_(http_fetcher),
      weak_factory_(this) {
}

QuicProxyStream::~QuicProxyStream() {}

void QuicProxyStream::SendRequest(const std::string& body) {
  DCHECK_EQ(backend_request_id_, kInvalidRequestId);

  stellite::HttpRequest backend_request;

  // build backend request header
  SpdyHeaderBlock* spdy_headers = request_headers();
  DCHECK(spdy_headers);

  // copy header
  HttpRequestHeaders backend_headers;
  ConvertSpdyHeaderToHttpRequest(*spdy_headers, HTTP2, &backend_headers);

  HttpRequest::RequestType method = ParseMethod(*spdy_headers, HTTP2);
  backend_request.request_type = method;

  // set x-forwarded-for header
  net::IPEndPoint remote_address = session()->connection()->peer_address();
  std::string remote_ip = remote_address.address().ToString();

  std::string xff;
  if (backend_headers.GetHeader(kHeaderXFF, &xff)) {
    backend_headers.SetHeader(kHeaderXFF, xff + "," + remote_ip);
  } else {
    backend_headers.SetHeader(kHeaderXFF, remote_ip);
  }

  // set x-forwarded-host header
  std::string host;
  if (backend_headers.GetHeader(kHeaderHost, &host)) {
    backend_headers.SetHeader(kHeaderXFH, host);
  }

  // set url
  GURL backend_request_url = GetProxyRequestURL(*spdy_headers);
  if (!backend_request_url.is_valid()) {
    SendErrorResponse(400, kBadRequest);
    return;
  }
  backend_request.url = backend_request_url.spec();

  // all response get streamized callback
  backend_request.is_stream_response = true;

  // set stop when response are redirect
  backend_request.is_stop_on_redirect = true;

  // check upload
  bool has_payload = bool(body.size()) || is_chunked_upload_;
  bool is_upload_request = (method == HttpRequest::PUT ||
                            method == HttpRequest::POST ||
                            method == HttpRequest::PATCH);
  if (is_upload_request && !has_payload) {
    SendErrorResponse(400, kBadRequest);
    return;
  }

  if  (!is_upload_request && has_payload) {
    SendErrorResponse(400, kBadRequest);
    return;
  }

  // set upload content
  if (is_upload_request) {
    if (is_chunked_upload_) {
      backend_request.is_chunked_upload = true;
    } else {
      backend_request.upload_stream.write(body.data(), body.size());
    }
  } else {
    // erase content-length header
    backend_headers.RemoveHeader(kHeaderContentLength);

    // erase transfer-encoding header
    backend_headers.RemoveHeader(kHeaderTransferEncoding);
  }

  // copy to backend_request
  backend_request.headers.SetRawHeader(backend_headers.ToString());

  backend_request_id_ = http_fetcher_->Request(backend_request,
                                               kBackendRequestTimeout,
                                               weak_factory_.GetWeakPtr());
  DCHECK_NE(backend_request_id_, kInvalidRequestId);
}

void QuicProxyStream::AppendChunkToUpload(const char* data, size_t len,
                                          bool fin) {
  http_fetcher_->AppendChunkToUpload(backend_request_id_,
                                     std::string(data, len), fin);
}

void QuicProxyStream::OnHeaderAvailable(bool fin) {
  SpdyHeaderBlock* headers = request_headers();

  base::StringPiece transfer_encoding =
      headers->GetHeader("transfer-encoding");
  is_chunked_upload_ =
      transfer_encoding.find("chunked") != base::StringPiece::npos;

  if (fin || content_length() == 0 || is_chunked_upload_) {
    SendRequest(std::string());
  }
}

void QuicProxyStream::OnContentAvailable(const char* data, size_t len,
                                         bool fin) {
  if (is_chunked_upload_) {
    DCHECK_NE(backend_request_id_, kInvalidRequestId);
    AppendChunkToUpload(data, len, fin);
    return;
  }

  if (len > 0) {
    payload_stream_.write(data, len);
  }

  if (fin) {
    SendRequest(payload_stream_.str());
  }
}

void QuicProxyStream::OnTaskComplete(int request_id,
                                     const URLFetcher* source,
                                     const HttpResponseInfo* response_info) {
  NOTREACHED();
}

void QuicProxyStream::OnTaskHeader(int request_id,
                                   const URLFetcher* source,
                                   const HttpResponseInfo* response_info) {
  DCHECK_EQ(request_id, backend_request_id_);

  if (source == nullptr) {
    SendErrorResponse();
    return;
  }

  scoped_refptr<HttpResponseHeaders> headers = source->GetResponseHeaders();
  if (headers == nullptr) {
    SendErrorResponse();
    return;
  }

  SpdyHeaderBlock res_headers;
  CreateSpdyHeadersFromHttpResponse(*headers, &res_headers);

  // set Server header
  res_headers[kHeaderServer] = "stellite/1.0";

  // because of proxy response content are plain-text, erase content-encoindg
  res_headers.erase("content-encoding");

  int64_t content_length = headers->GetContentLength();
  bool send_fin = !(headers->IsChunkEncoded() || content_length > 0);
  WriteHeaders(std::move(res_headers), send_fin, nullptr);
}

// backend -> quic server
void QuicProxyStream::OnTaskStream(int request_id,
                                   const char* data, size_t len, bool fin) {
  DCHECK_EQ(request_id, backend_request_id_);
  base::StringPiece body(data, len);
  WriteOrBufferData(body, fin, nullptr);
}

void QuicProxyStream::OnTaskError(int request_id,
                                  const URLFetcher* source,
                                  int error_code) {
  DCHECK_EQ(request_id, backend_request_id_);
  SendErrorResponse();
}

GURL QuicProxyStream::GetProxyRequestURL(const SpdyHeaderBlock& headers) {
  GURL request_url(proxy_pass_);

  SpdyHeaderBlock::const_iterator it = headers.find(":path");
  if (it == headers.end()) {
    return GURL();
  }

  GURL::Replacements repl;
  repl.SetPathStr(it->second.as_string());
  return request_url.ReplaceComponents(repl);
}

}  // namespace net
