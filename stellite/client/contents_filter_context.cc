// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stellite/client/contents_filter_context.h"

#include "net/url_request/url_request_context.h"

namespace net {

ContentsFilterContext::ContentsFilterContext()
    : is_cached_content_(false),
      ok_to_call_get_url_(true),
      response_code_(-1),
      context_(new URLRequestContext()) {
}

ContentsFilterContext::~ContentsFilterContext() {}

void ContentsFilterContext::NukeUnstableInterfaces() {
  context_.reset();
  ok_to_call_get_url_ = false;
  request_time_ = base::Time();
}

bool ContentsFilterContext::GetMimeType(std::string* mime_type) const {
  *mime_type = mime_type_;
  return true;
}

// Which URL was used to access this data?
// Return false if GURL is not present.
bool ContentsFilterContext::GetURL(GURL* gurl) const {
  DCHECK(ok_to_call_get_url_);
  *gurl = gurl_;
  return true;
}

// What was this data requested from a server?
base::Time ContentsFilterContext::GetRequestTime() const {
  return request_time_;
}

bool ContentsFilterContext::IsCachedContent() const { return is_cached_content_; }

SdchManager::DictionarySet*
ContentsFilterContext::SdchDictionariesAdvertised() const {
  return dictionaries_handle_.get();
}

int64_t ContentsFilterContext::GetByteReadCount() const { return 0; }

int ContentsFilterContext::GetResponseCode() const { return response_code_; }

const URLRequestContext* ContentsFilterContext::GetURLRequestContext() const {
  return context_.get();
}

const NetLogWithSource& ContentsFilterContext::GetNetLog() const {
  return net_log_;
}

}  // namespace net
