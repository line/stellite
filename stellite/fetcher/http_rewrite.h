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

#ifndef STELLITE_FETCHER_HTTP_REWRITE_H_
#define STELLITE_FETCHER_HTTP_REWRITE_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "components/url_matcher/regex_set_matcher.h"

namespace url_matcher {
class StringPattern;
}

namespace net {

typedef std::vector<std::pair<std::string, std::string>> RewriteRules;

class HttpRewrite {
 public:
  HttpRewrite();
  ~HttpRewrite();

  void AddRule(const std::string& pattern, const std::string& replace);
  void RecompilePattern();
  void Clear();

  // Rewrite if matching a rule
  bool Rewrite(const std::string& origin_path,
               std::string* replaced_path) const;

  static HttpRewrite* Create(const RewriteRules& rules);

 private:
  std::vector<const url_matcher::StringPattern*> pattern_list_;
  std::vector<std::string> replace_format_;
  url_matcher::RegexSetMatcher matcher_;

  DISALLOW_COPY_AND_ASSIGN(HttpRewrite);
};

} // namespace net

#endif  // STELLITE_FETCHER_HTTP_REWRITE_H_
