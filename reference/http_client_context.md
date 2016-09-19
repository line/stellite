# class HttpClientContext
> HTTP 요청에 필요한 context를 들고 있는 클래스다. 내부적으로 thread를 가지고 있고 이것을 사용하여 HTTP요청을 처리한다.

# HttpClientContext::Params
```c++
  struct Params {
    Params();
    ~Params();

    bool using_quic;
    bool using_spdy;
    bool using_http2;
    bool using_quic_disk_cache;
    std::string proxy_host;

    std::string origin_to_force_quic_on;
  };

```
* |Params.using\_quic|
 * option for using quic protocol
 * default value: false

* |Params.using\_spdy|
 * option for using spdy protocol
 * defualt value: false

* |Parmas.using\_http2|
 * option for using http2
 * default value: false

* |Params.using\_quic\_disk\_cache|
 * quic server config are cached on the disk. it was vary useful when http client try 0-RTT connection request.
 * default value: false

* |Params.proxy\_host|
 * set http client forward proxy
 * The type of |Params.proxy_host| is |std::string|. when this value are setted, http client every request send to |Params.proxy\_pass| origin
 * default value: ""

* |Params.origin\_to\_force\_quic\_on|
 * force to using QUIC protocol when request URL origin are same to this value
 * default value: ""

## HttpClientContext::HttpClientContext
##### Creator of HttpClientContext
> HttpClientContext::HttpClientContext(const Params& params)
> > Paramerters:
> > > HttpClientContext::Params

> > Returns:

## HttpClientContext::Initialize
##### initialize a context below on background thread
> bool HttpClientContext::Initialize()
> > Parameters:

> > Returns:
> > > boolean value that either success or not.

- create network thread
- initialize hostname resolver
- initialize SSL config service
- initialize cert verifier
- initialize proxy service
- initialize protocol (HTTP, HTTPS, SPDY, HTTP2, QUIC)

## HttpClientContext::TearDown
##### release all resource on context on background thread
> bool HttpClientContext::TearDown()
> > Parameters:

> > Returns:
> > > boolean value that either shutdown is success or not.

## HttpClientContext::Cancel
##### cancel all request that spawned client from http client context
> bool HttpClientContext::Cancel()
> > Parameters:

> > Returns:
> > > boolean value that cancel all request are success or not.

## HttpClientContext::CreateHttpClient
##### The faction function that generate HttpClient interface object.
> HttpClient* HttpClientContext::CreateHttpClient()
> > Parameters:

> > Returns:
> > > HttpClient*: http client

## HttpClientContext::ReleaseHttpClient
##### release http client on background thread
> HttpClientContext::ReleaseHttpClient(HttpClient* client)
> > Parameters:
> > > HttpClient* : release target

> > Returns:

## HttpClientContext::InitVM
##### initialize android vm. this function are relate SSL certification verifier
> static void HttpClientContext::InitVM()
> > Parameters:

> > Returns:

## HttpClientContext::InitContext
##### register android context to c++ httpclientcontext
> static void HttpClientContext::InitContext(JNIEnv* env, jobject context)
> > Parameters:
> > > JNIEnv* : android applications JNIEnv

> > > jobject: android application's activity context

> > Returns:

