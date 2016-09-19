# interface HttpResponse
> http request 를 하기 위한 기본적인 interface를 제공한다.

```c++

namespace stellite {

class HttpResponseHeader
    : public RefCountedThreadSafe<HttpResponseHeader> {
 public:
  HttpResponseHeader(const std::string& raw_header);
  HttpResponseHeader();
  virtual ~HttpResponseHeader();

  bool EnumerateHeaderLines(void** iter,
                            std::string* name,
                            std::string* value) const;

  bool EnumerateHeader(void** iter,
                       const std::string& name,
                       std::string* value) const;

  bool HasHeader(const std::string& name) const;

  const std::string& raw_headers() const;

private:
  class HttpResponseHeaderImpl;
  up<HttpResponseHeaderImpl> impl_;
};

// clone of net/http/http_response_info.h
struct HttpResponse {
  enum ConnectionInfo {
    CONNECTION_INFO_UNKNOWN = 0,
    CONNECTION_INFO_HTTP1 = 1,
    CONNECTION_INFO_DEPRECATED_SPDY2 = 2,
    CONNECTION_INFO_SPDY3 = 3,
    CONNECTION_INFO_HTTP2 = 4,
    CONNECTION_INFO_QUIC1_SDPY3 = 5,
    CONNECTION_INFO_HTTP2_14 = 6,
    CONNECTION_INFO_HTTP2_15 = 7,
    NUM_OF_CONNECTION_INFOS,
  };

  HttpResponse();
  HttpResponse(const HttpResponse& other);
  ~HttpResponse();

  HttpResponse& operator=(const HttpResponse& other);

  int response_code;
  long long content_length;

  std::string status_text;
  std::string mime_type;
  std::string charset;

  bool was_cached;
  bool server_data_unavailable;
  bool network_accessed;
  bool was_fetched_via_spdy;
  bool was_npn_negotiated;
  bool was_fetched_via_proxy;

  ConnectionInfo connection_info;
  std::string connection_info_desc;

  sp<HttpResponseHeader> headers;
};

} // namespace stellite

```

* response_code : http response code
* content_length : http response content length (bytes)

* status_text : http response status text
* mime_type : http response
* charset : http response charset

* was_cached : cached response or not
* server_data_unavailable :
* network_accessed :
* was_fetched_via_spdy : spdy result
* was_npn_negotiated : next protocol negotiation processed or not
* was_fetched_via_proxy : is proxy response or not

* response_headers : this is http response header dictionary

* connection_info : protocol identifier (HTTP, HTTPS, SPDY, HTTP2, QUIC)
* connection_info_desc : string description for protocol

