# How to build

### Requirements

##### Mac (Darwin)
Target platform: Android, iOS, Darwin, Linux
* Python 2.7
* Git
* Docker Toolbox 1.11.1+ (for Android)
* VirtualBox (for Docker Toolbox)

##### Linux (Ubuntu)
Target platform: Android, Linux
* Python 2.7
* Git

### Build script options

```bash
$ ./tools/build.py --help
usage: build.py [-h] [--target-platform {linux,android,ios,mac,windows}]
                [--target {stellite_quic_server,stellite_http_client,trident_http_client,client_binder}]
                [--target-type {static_library,shared_library,executable}]
                [-v]
                {clean,build}

positional arguments:
  {clean,build}

optional arguments:
  -h, --help            show this help message and exit
  --target-platform {linux,android,ios,mac,windows}
                        default platform mac
  --target {stellite_quic_server,stellite_http_client,trident_http_client,client_binder}
  --target-type {static_library,shared_library,executable}
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
