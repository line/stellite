//
// 2016 write by snibug
//

#ifndef STELLITE_FETCHER_HTTP_FETCHER_DELEGATE_H_
#define STELLITE_FETCHER_HTTP_FETCHER_DELEGATE_H_

#include <stddef.h>
#include <stdint.h>

#include "stellite/include/stellite_export.h"

namespace net {
class HttpResponseInfo;
class URLFetcher;
}

namespace stellite {

class STELLITE_EXPORT HttpFetcherDelegate {
 public:
  virtual ~HttpFetcherDelegate() {}

  virtual void OnFetchComplete(const net::URLFetcher* source,
                               const net::HttpResponseInfo* response_info) = 0;

  virtual void OnFetchStream(const net::URLFetcher* source,
                             const net::HttpResponseInfo* response_info,
                             const char* data, size_t len, bool fin) = 0;
};

}  // namespace net

#endif  // STELLITE_FETCHER_HTTP_FETCHER_DELEGATE_H_
