# interface HttpRequest
> http request 를 하기 위한 기본적인 interface를 제공한다.

```c++

namespace stellite {

class HttpRequestHeader {
 public:
  explicit HttpRequestHeader();
  virtual ~HttpRequestHeader();

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

struct HttpRequest {
  explicit HttpRequest();
  ~HttpRequest();

  std::string url;
  std::string method;
  std::stringstream upload_stream;

  std::unique_ptr<HttpRequestHeader> headers;
};

} // namespace stellite

```

* url : request url
* method : request method
* upload_stream : request http body
* request_headers : request header dictionary

