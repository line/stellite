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
usage: build.py [-h] [--chromium-path CHROMIUM_PATH] [--out OUT]
                --target-platform {ios,android,darwin,linux} --target
                {stellite,quic_server,client_binder}
                [--target-type {static_library,shared_library}]
                [--cache-dir CACHE_DIR]
                {clean,build,clean_build,unittest,sync_chromium}

positional arguments:
  {clean,build,clean_build,unittest,sync_chromium}
                        build action

optional arguments:
  -h, --help            show this help message and exit
  --chromium-path CHROMIUM_PATH
                        specify a chromium path
  --out OUT             build output path
  --target-platform {ios,android,darwin,linux}
                        ios, android, darwin, linux
  --target {stellite,quic_server,client_binder}
                        stellite library or quic server
  --target-type {static_library,shared_library}
                        library type
  --cache-dir CACHE_DIR
```

### Build example

Client build:
```bash
./tools/build.py --target-platform=android --target stellite build
```

Server build:
```bash
./tools/build.py --target-platform=linux --target quic_server build
```
