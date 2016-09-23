# Client library

C++ client library to use with the Chromium's QUIC protocol module. The library has four interfaces as follows.

* [HttpClientContext](./include/http_client_context.h)
* [HttpClient](./include/http_client.h)
* [HttpRequest](./include/http_request.h)
* [HttpResponse](./include/http_response.h)

---


# HttpClientContext


Represents a class that contains context information required to process HTTP requests. It has factory functions for creating (handle, manipulate, control) HttpClient xxx. As it uses a network thread, it requires explicit initialization and destroyer function.

```c++
class STELLITE_EXPORT HttpClientContext {
 public:
  struct Params {
    Params();
    Params(const Params& other);
    ~Params();

    bool using_quic;
    bool using_spdy;
    bool using_http2;
    bool using_quic_disk_cache;
    std::string proxy_host;
    std::string origin_to_force_quic_on;
  };

  explicit HttpClientContext(const Params& params);
  virtual ~HttpClientContext();

  bool Initialize();
  bool TearDown();
  bool Cancel();

  // the factory function but HttpClient object ownership was on
  // HttpClientContext. so do release client with ReleaseHttpClient function
  // not using delete
  HttpClient* CreateHttpClient(HttpResponseDelegate* visitor);
  void ReleaseHttpClient(HttpClient* client);

#if defined(USE_OPENSSL_CERTS)
  static bool ResetCertBundle(const std::string& pem_path);
#endif

#if defined(ANDROID)
  // In Android JNI applications, InitVM function must call
  // jint JNI_OnLoad(JavaVM*vm, void* reserved) initialized function
  static bool InitVM(JavaVM* vm);
#endif

 private:
  class HttpClientContextImpl;
  HttpClientContextImpl* impl_;

  // DISALLOW_COPY_AND_ASSIGN
  HttpClientContext(const HttpClientContext&);
  void operator=(const HttpClientContext&);
};
```

### Interface

| Function | Description |
| --- | --- |
| CreateHttpClient | A factory function that creates an HttpClient object. HttpClientContext has the ownership of the object. |
| ReleaseHttpClient | Releases an HttpClient object. The object is not deleted directly because HttpClientContext has its ownership. |
| Initialize | Initializes context xx on a background thread, such as hostname resolver, SSL config service, certificate verifier, proxy service, and protocols. |
| TearDown | Releases all context resources on a background thread.|
| Cancel | Cancels all requests that have generated HttpClient objects from HttpClientContext.|
| ResetCertBundle | Use this function to reset CA certificate if you want to enable the OpenSSL option in the Stellite build. |
| InitVM | An initialization function used in a Android system. This function must be called when an application begins.|

### Parameter

|Option | Command | Default value | Example |
| --- | --- | --- | --- |
| `using_quic` | Use a QUIC protocol. When set to true, (주어) connects to QUIC server via QUIC Discovery. When Alt-Svc header is specified as quic="hostname:port", QUIC Discovery establishes a QUIC connection using the specified QUIC server hostname and the UDP port.  | false | using_quic = true |
| `using_spdy` | Use a SPDY 3.1 protocol. When set to true, NPN (Next Protocol Negotiation) attempts to establish a SPDY connection.  | false | using_spdy = true |
| `using_http2` | Use an HTTP2 protocol. When set to true, NPN (Next Protocol Negotiation) and ALPN (Application Layer Protocol Negotiation) attempt to establish an HTTP2 connection.  | false | using_http2 = true |
| `using_quic_disk_cache` | Store QUIC server information on a disk cache. When set to true, a file cache is stored in the .http_cache directory within a mobile sandbox(?). When server information is cached, a client can attempt to make a request to a server with 0-RTT. | false | using_quic_disk_cache = true |
| `proxy_host` | Allow a client to use a proxy. When proxy_host is set to "http://127.0.0.1:9000", (주어) attempts to connect to 127.0.0.1 number 9000 proxy server. If the URL used for the request is https://stellite.io, it is a proxy server, not an HTTP client, that makes the connection and processes the request. | "" | proxy_host = "http://127.0.0.1:8080" |
| `origin_to_force_quic_on` | Force to use a QUIC protocol. If you specify "stellite.com:443" and make a client request to URL https://stellite.io:443, you can directly (동사) to a QUIC server. If you want to use this URL, you need to provide stellite.io certificate and key file to a QUIC server. | "" | origin_to_force_quic_on = "https://stellite.io:443" |

---

# HttpClient

## HttpResponseDelegate

 Represents an interface that invokes client callbacks for HTTP responses.

```c++
class STELLITE_EXPORT HttpResponseDelegate {
 public:
  virtual ~HttpResponseDelegate() {}

  virtual void OnHttpResponse(int request_id, const HttpResponse& response,
                              const char* body, size_t body_len)=0;

  virtual void OnHttpStream(int request_id, const HttpResponse& response,
                            const char* stream_data, size_t stream_len)=0;

  // The error code are defined at net/base/net_error_list.h
  virtual void OnHttpError(int request_id, int error_code,
                           const std::string& error_message)=0;
};
```

### Interface

| Function | Description |
| --- | --- |
| OnHttpResponse()| A callback function invoked asynchronously from a network thread when a response to HttpClient::Request() is available (ready). |
| OnHttpStream()| A callback function invoked asynchronously from a network thread when a response to HttpClient::Stream() is available (ready). |
| OnHttpError()| A callback function invoked asynchronously when an error occurs after calling HttpClient::Request() or HttpClient::Stream(). |

### Parameter

| Parameter| Type | Description |
| --- | --- | --- |
| request_id | int | An identifier used to identify a request for HttpClient::Request(). For example, from which request the response has come from. |
| response | HttpResponse | HTTP response header information |
| body | char\* | HTTP response data pointer |
| body_len | size_t | HTTP response data size |
| stream | char\* | HTTP response stream pointer |
| stream_len | char\* | HTTP response stream data size |



## HttpClient 

Represents a client that handles HTTP requests. It selects a protocol depending on the parameter options specified in HttpClientContext.

```c++
// Interface of HTTP request or stream (long-poll, chunked-response)
class STELLITE_EXPORT HttpClient {
 public:
  virtual ~HttpClient() {}

  // The delegate owned by HttpClient
  virtual int Request(const HttpRequest& request)=0;

  // The delegate owned by HttpClient
  virtual int Request(const HttpRequest& request, int timeout)=0;

  // The delegate owned by HttpClient
  virtual int Stream(const HttpRequest& request)=0;

  // The delegate owned by HttpClient
  virtual int Stream(const HttpRequest& request, int timeout)=0;
};
```

### Interface

| Function | Description |
| --- | --- |
| Request() | An interface for sending HTTP requests. A callback is invoked only one time. |
| Stream() | An interface for sending HTTP requests. A callback may be invoked multiple times. It can be useful for  a chunked response or a long-poll request. |

### Parameter

|  Parameter | Type | Description |
| --- | --- | --- |
| request | HttpRequest | Request information consisting of request URL, method, and header data. |
| timeout | int | The amount of time an HTTP request waits for a response. There are connection timeout and read timeout. If an HttpClient object does not receive a response within the specified timeout, an error callback is invoked.|


---

# HttpRequest

Represents an interface for making an HTTP request.

## HttpRequestHeader

### Use example

```c++
// The proxy of net::HttpRequestHeaders
class STELLITE_EXPORT HttpRequestHeader {
 public:
  explicit HttpRequestHeader();
  HttpRequestHeader(const HttpRequestHeader& other);
  ~HttpRequestHeader();

  void SetRawHeader(const std::string& raw_header);

  // HTTP header modifier
  bool HasHeader(const std::string& key) const;
  bool GetHeader(const std::string& key, std::string* value) const;
  void SetHeader(const std::string& key, const std::string& value);
  void RemoveHeader(const std::string& key);
  void ClearHeader();

  std::string ToString() const;

 private:
  class HttpRequestHeaderImpl;
  std::unique_ptr<HttpRequestHeaderImpl> impl_;
};
```

### Interface

| Function | Description |
| --- | --- |
| SetRawHeader() | Initializes an HttpHeader with the header's full-text. It contains a delimiter [CR]LR and can convert to a key-value data structure. |
| HasHeader() | Checks whether a header exists or not |
| GetHeader() | Gets a header value |
| SetHeader() | Sets a header value |
| RemoveHeader() | Removes a header value of a particular key |
| ClearHeader() | Clears a header value for all the keys |
| ToString() | Converts all header data to a raw-header string |

### Parameter

| Name | Type | Description |
| --- | --- | --- |
| raw_header | std::string | Full-text of header data. It contains a delimiter and key-value pairs. |
| key | std::string | HTTP header key|
| value | std::string | HTTP header value |


## HttpRequest

### Use Example

```c++
struct STELLITE_EXPORT HttpRequest {
  explicit HttpRequest();
  ~HttpRequest();

  std::string url;
  std::string method;
  std::stringstream upload_stream;

  HttpRequestHeader headers;
};
```

### Parameter

| Name | Type | Description |
| --- | --- | --- |
| url | std::string | HTTP request URL (for example, https://example.com) |
| method | std::string| HTTP request method (GET, POST, PUT, DELETE)|
| upload_stream | std::stringstream | Payload data, HTTP request body |
| headers | HttpRequestheader |HTTP request header directory |

---

# HttpResponse

Represents an interface for receiving a response to an HTTP request.

## HttpResponseHeader


### Use example 

```c++
class STELLITE_EXPORT HttpResponseHeader {
 public:
  HttpResponseHeader();
  HttpResponseHeader(const HttpResponseHeader& other);
  HttpResponseHeader(const std::string& raw_header);
  virtual ~HttpResponseHeader();

  HttpResponseHeader& operator=(const HttpResponseHeader& other);

  // HTTP header enumerator that knows about http/1.1 header key and value
  bool EnumerateHeaderLines(size_t* iter,
                            std::string* name,
                            std::string* value) const;

  // HTTP header enumerator value that is related to a header key
  bool EnumerateHeader(size_t* iter,
                       const std::string& name,
                       std::string* value) const;

  // Check whether a header exists or not
  bool HasHeader(const std::string& name) const;

  // Return raw headers
  const std::string& raw_headers() const;

  void Reset(const std::string& raw_header);

private:
  class HttpResponseHeaderImpl;
  std::unique_ptr<HttpResponseHeaderImpl> impl_;
};
```

### Interface

| Function | Description |
| --- | --- |
| EnumerateHeaderLines() | Enumerates all header elements line by line to check their header key and value. |
| EnumerateHeader() | Enumerates a header of a particular key to check its value. |
| HasHeader() | Verifies whether a header value exists for a particular key. |
| raw_headers() | Generates a single string that contains all header information. |
| Reset() | Resets all header information.|

### Parameter

| Name | Type | Description |
| --- | --- | --- |
| iter |size_t\* | An iterator that enumerates headers |
| name | std::string | A key name of an HTTP header |
| raw_header| std::string| Raw data of an HTTP header |

## HttpResponse

```c++
struct STELLITE_EXPORT HttpResponse {
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

  HttpResponseHeader headers;
};
```

### Parameter

|  Name | Type | Description|
| --- | --- | --- |
| response_code | int | HTTP response code |
| content_length | long long | HTTP body length |
| status_text | std::string | HTTP response status |
| mime_type | std::string | HTTP response MIME type |
| charset | std::string | A character set of an HTTP response body (for example, UTF-8)|
| was_cached | bool| Whether it is a cached response or not. When true, it is a cached response. |
| server_data_unavailable | bool| A server error |
| network_accessed | bool | Whether a response was sent over a network or not (as opposed to a cached response). When true, it is a network response. |
| was_fetched_via_spdy | bool | When true, a response was made over a SPDY protocol. |
| was_npn_negotiated | bool | Whether NPN negotiation was used for a response or not. |
| was_fetched_via_proxy | bool | Whether a response was fetched via a proxy or not. |
| connection_info | ConnectionInfo | |
| connection_info_desc| std::string ||
| headers | HttpResponseHeader ||

