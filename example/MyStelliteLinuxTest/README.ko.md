<!-- TOC -->

- [소개](#소개)
- [환경, 종속성](#환경-종속성)
- [빌드](#빌드)
- [테스트](#테스트)

<!-- /TOC -->

<br>
<br>
<br>

# 소개
- stellite의 QUIC GET 기능만 간단히 확인할 수 있는 샘플.
- linux(ubuntu 16.04)에서 googletest를 이용하여 동작함.
    - ios, android 버전의 동작을 확인하려면 환경이 복잡하므로 만들게 되었다.
- "libstellite_http_client.so" 을 미리 git에 올려두었음.
    - stellite 빌드가 상당히 오래 걸리고 여러가지 환경의 영향을 많이 받기 때문에 이렇게 함.
    - static library(.a 파일)은 링크오류로 이 샘플에서는 shared library(.so파일) 을 사용함.
        - 원인파악안됨. ㅠㅠ
- "libgtest.a" 도 미리 빌드해서 git에 올려두었음.
    - ubuntu 버전이 변경되면 링크오류가 날수도 있는데 이렇다면 googletest 프로젝트를 내려받아서 새로 빌드해서 사용하면 해결됨.

<br>
<br>
<br>

# 환경, 종속성
- ubuntu 16.04
- cmake

<br>
<br>
<br>

# 빌드

```bash
$ cmake .; make -j8;
-- The C compiler identification is GNU 5.4.0
-- The CXX compiler identification is GNU 5.4.0
-- Check for working C compiler: /usr/bin/cc
-- Check for working C compiler: /usr/bin/cc -- works
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Detecting C compile features
-- Detecting C compile features - done
-- Check for working CXX compiler: /usr/bin/c++
-- Check for working CXX compiler: /usr/bin/c++ -- works
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Detecting CXX compile features
-- Detecting CXX compile features - done
GOOGLETEST LIB PATH : /home/hhd/stellite/example/MyStelliteLinuxTest/googletest/bin/libgtest.a
STELLITE LIB PATH : /home/hhd/stellite/example/MyStelliteLinuxTest/stellite/bin/libstellite_http_client.so
-- Configuring done
-- Generating done
-- Build files have been written to: /home/hhd/stellite/example/MyStelliteLinuxTest
Scanning dependencies of target MyStelliteLinuxTest
[ 33%] Building CXX object CMakeFiles/MyStelliteLinuxTest.dir/MyStelliteHttpCallback.cpp.o
[ 66%] Building CXX object CMakeFiles/MyStelliteLinuxTest.dir/stellite_get_quic_unittest.cpp.o
[100%] Linking CXX executable MyStelliteLinuxTest
ldconfig ...
[sudo] password for hhd:
[100%] Built target MyStelliteLinuxTest



$ ll ./MyStelliteLinuxTest
-rwxrwxr-x 1 hhd hhd 1085680  6월 14 15:16 ./MyStelliteLinuxTest*
```

<br>
<br>
<br>

# 테스트

```bash
$ ./MyStelliteLinuxTest
[==========] Running 1 test from 1 test case.
[----------] Global test environment set-up.
[----------] 1 test from stellite
[ RUN      ] stellite.get_quic
request.url[▒:Q▒G7]
OnHttpResponse.OnHttpResponse request_id[1] body_len[50510] connection_info_desc[quic/1+spdy/3] connection_info[5]
OnHttpResponse.OnHttpResponse body[<!doctype html><html itemscope="" itemtype="http://schema.org/WebPage" lang="ko"><head><meta content="text/html; charset=UTF-8" 
...
```
