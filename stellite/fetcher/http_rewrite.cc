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

#include "stellite/fetcher/http_rewrite.h"

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "components/url_matcher/string_pattern.h"
#include "components/url_matcher/url_matcher.h"
#include "third_party/re2/src/re2/re2.h"

namespace net {

const int kMaxGroupCount = 16;

HttpRewrite::HttpRewrite() {
}

HttpRewrite::~HttpRewrite() {}

void HttpRewrite::AddRule(const std::string& pattern,
                          const std::string& replace) {
  url_matcher::StringPattern::ID id =
      static_cast<url_matcher::StringPattern::ID>(pattern_list_.size());
  url_matcher::StringPattern* string_pattern =
      new url_matcher::StringPattern(pattern, id);
  pattern_list_.push_back(string_pattern);

  replace_format_.push_back(std::string(replace));
}

void HttpRewrite::RecompilePattern() {
  matcher_.ClearPatterns();
  matcher_.AddPatterns(pattern_list_);
}

void HttpRewrite::Clear() {
  pattern_list_.clear();
  replace_format_.clear();
  matcher_.ClearPatterns();
}

bool HttpRewrite::Rewrite(const std::string& origin_path,
                          std::string* replaced_path) const {
  DCHECK(replaced_path);

  std::set<url_matcher::StringPattern::ID> result;
  if (!matcher_.Match(origin_path, &result)) {
    return false;
  }

  url_matcher::StringPattern::ID pattern_id = *result.begin();
  const std::string& pattern();

  re2::RE2 re(pattern_list_[pattern_id]->pattern());
  int group_count = re.NumberOfCapturingGroups();
  if (group_count >= kMaxGroupCount) {
    LOG(ERROR) << "Overflow in rewriting matcher group count: " << group_count;
    return false;
  }

  re2::RE2::Arg argv[kMaxGroupCount];
  std::vector<std::string> group(kMaxGroupCount);
  for (uint32_t i = 0; i < arraysize(argv); ++i) {
    argv[i] = &group[i];
  }

  const re2::RE2::Arg* const args[sizeof(argv)] = {
    &argv[0], &argv[1], &argv[2], &argv[3],
    &argv[4], &argv[5], &argv[6], &argv[7],
    &argv[8], &argv[9], &argv[10], &argv[11],
    &argv[12], &argv[13], &argv[14], &argv[15],
  };

  if (!re2::RE2::FullMatchN(origin_path, re, args, group_count)) {
    return false;
  }

  std::string replace = replace_format_[pattern_id];
  for (int i = 0; i < group_count; ++i) {
    std::string from(std::string("\\$") + base::IntToString(i + 1));
    re2::RE2::GlobalReplace(&replace, from, group[i]);
  }
  replaced_path->swap(replace);

  return true;
}

// Static
HttpRewrite* HttpRewrite::Create(const RewriteRules& rules) {
  std::unique_ptr<HttpRewrite> http_rewrite(new HttpRewrite());
  for (size_t i = 0; i < rules.size(); ++i) {
    http_rewrite->AddRule(rules[i].first, rules[i].second);
  }
  http_rewrite->RecompilePattern();
  return http_rewrite.release();
}


} // namespace net
