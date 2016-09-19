# Welcome to Stellite

Stellite project is an open-source library that offers an easy way to develop, build, and implement client/server running primarily over the QUIC protocol, a network protocol developed by Google as part of the Chromium project. It aims to provide fast and stable internet connection for mobile applications.

Stellite is an open-source project developed by [LINE Corporation](http://linecorp.com/en/) based on the Chromium project.

Licensed under [Apache License 2.0](http://www.apache.org/licenses/LICENSE-2.0).

## Why QUIC?

Today, mobile application developers are struggling with challenges posed by constantly changing network environments; long connection time (RTT overhead), packet loss, handover failures, head-of-line blocking, duplicate congestion window, to name a few.

QUIC is a new transport protocol that can solve these problems by providing essential featrues as follows.

* Dramatically reduced connection establishment time
* Improved congestion control
* Multiplexing without head-of-line blocking
* Forward error correction
* Connection migration
* TLS-level security

See [QUIC docs](https://www.chromium.org/quic) by Google for more details.

## Stellite project stack

Stellite consists of two components: QUIC server and client library.


<img src="./res/architecture_stellite.png">

<img src="./res/architecture_client.jpg">


### QUIC server

QUIC server is an extension of the Chromium's QUIC simple server. It uses the QUIC protocol to process HTTP requests.

* Supports reverse proxy
* Supports multi-threading for enhanced performance
* Supports file-based logging

### Client library

Client library provides a protocol negotiation layer that can help a web service choose an appropriate protocol and reduce latency.

* Supports HTTP(S), SPDY, HTTP2, QUIC
* Easy to build, link, and integrate with mobile applications
* Includes a thread model to run network on a thread
* Uses forward declarations to avoid exposing Chromium interface (No need to worry about implementing Chromium)

## Getting started

* [How to build](./BUILD.md)
* [Client guide](./CLIENT_GUIDE.md)
* [Server guide](./SERVER_GUIDE.md)

## Read more

* [Stellite design document (Client-side)] (./DESIGN.md)
* Release notes
* [Contributing](./CONTRIBUTE.md)
* [About QUIC](https://www.chromium.org/quic)


## License

Copyright 2016 LINE Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
