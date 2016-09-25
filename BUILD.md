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
                --target-platform {ios,android,darwin,linux}
                {stellite,quic_server}

positional arguments:
  {stellite,quic_server}
                        specify build target

optional arguments:
  -h, --help            show this help message and exit
  --chromium-path CHROMIUM_PATH
                        specify a chromium path
  --out OUT             build output path
  --target-platform {ios,android,darwin,linux}
                        ios, android, darwin, linux
```

### Build example

Client build:
```bash
./tools/build.py --target-platform=android stellite
```

Server build:
```bash
./tools/build --target-platform=linux quic_server
```


