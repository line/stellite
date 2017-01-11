# How to build

### Requirements

##### Mac (Darwin)
Target platform: Android, iOS, Darwin, Linux
* Python 2.7
* Git
* Docker Toolbox 1.11.1+ (for Android)
* VirtualBox (for Docker Toolbox)

##### Linux (Ubuntu 14.04 LTS)
Target platform: Android, Linux
* Python 2.7
* Git

### Build script options

```bash
$ ./tools/build.py --help
usage: build.py [-h] [--target-platform {linux,android,ios,mac,windows}]
                [--target {stellite_quic_server_bin,stellite_http_client,client_binder,stellite_http_client_bin,stellite_http_session_bin,simple_chunked_upload_client_bin}]
                [--target-type {static_library,shared_library,executable}]
                [--chromium-path CHROMIUM_PATH] [-v]
                {clean,build,unittest}

positional arguments:
  {clean,build,unittest}

optional arguments:
  -h, --help            show this help message and exit
  --target-platform {linux,android,ios,mac,windows}
                        default platform mac
  --target {stellite_quic_server_bin,stellite_http_client,client_binder,stellite_http_client_bin,stellite_http_session_bin,simple_chunked_upload_client_bin}
  --target-type {static_library,shared_library,executable}
  --chromium-path CHROMIUM_PATH
  -v, --verbose         verbose
```

### Build example

Client build:
```bash
./tools/build.py --target-platform=android --target stellite_http_client build
```

Server build:
```bash
./tools/build.py --target-platform=linux --target stellite_quic_server build
```
