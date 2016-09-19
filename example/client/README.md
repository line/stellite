STELLITE CLIENT EXAMPLE
=========

Build Step(MAC)
---

1. cd $(CHROMIUM_SRC)/stellite/build
2. ./mac.py build
3. cd $(CHROMIUM_SRC)/stellite/example/client
4. clang++ client.cc -fobjc-arc -I../../../liblinehttp/include -o client -llinehttp -lresolv -L../../../liblinehttp/prebuilt/mac -framework SystemConfiguration -framework ApplicationServices -framework IOKit -framework Security -framework AppKit -stdlib=libc++ -lstdc++ -Wno-c++11-extensions -std=c++11
5. ./client
6. input https://www.google.com and enter
7. Check response message

Build step(Linux.ubuntu)
---

comming soon ~
