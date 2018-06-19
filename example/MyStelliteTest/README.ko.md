# stellite android http, quic sample application

<!-- TOC -->

- [stellite android http, quic sample application](#stellite-android-http-quic-sample-application)
    - [개요](#개요)
    - [libstellite_http_client.so 를 빌드](#libstellite_http_clientso-를-빌드)
    - [NDK 사용설정된 android project 생성](#ndk-사용설정된-android-project-생성)
    - [libstellite_http_client.so 를 복사](#libstellite_http_clientso-를-복사)
    - [chromium 에서 빌드된 jar 파일들 복사](#chromium-에서-빌드된-jar-파일들-복사)
    - [NDK 빌드스크립트 작성](#ndk-빌드스크립트-작성)
    - [stellite 초기화 코드 작성](#stellite-초기화-코드-작성)
    - [stellite quic 요청/응답 코드 작성](#stellite-quic-요청응답-코드-작성)
    - [logcat으로 결과확인](#logcat으로-결과확인)

<!-- /TOC -->

## 개요
- android 에서 stellite를 NDK의 jni binding을 이용하여 사용하는 예제
- 버튼을 클릭하면 https://www.google.co.kr:443 를 QUIC 프로토콜을 이용하여 요청/응답 수행한다.
- 요청/응답의 정보들의 확인은 logcat을 확인하도록 한다.

## libstellite_http_client.so 를 빌드
- https://github.com/line/stellite/blob/master/BUILD.md

```bash
./tools/build.py --target-platform=android --target stellite_http_client --target-type shared_library build
```

- 180518 현재 chromium 빌드 종속성의 약간의 변경사항이 있어 빌드가 실패한다면 약간의 수동패치가 필요하다.
    - https://github.com/line/stellite/issues/58


## NDK 사용설정된 android project 생성
- android studio 의 기본 ndk 템플릿 이용해서 생성함.

## libstellite_http_client.so 를 복사
- android 타겟 빌드된 경로에 가보면 아키텍쳐별로 so 파일들이 생성되어 있음.
- 여기서는 샘플을 간단하게 만들기 위해서 armeabi-v7a 만 복사함.

```bash
./build_android/src/out_android_armv6/obj/libstellite_http_client.so
./build_android/src/out_android_x64/obj/libstellite_http_client.so
./build_android/src/out_android_armv7/obj/libstellite_http_client.so
./build_android/src/out_android_x86/obj/libstellite_http_client.so
./build_android/src/out_android_arm64/obj/libstellite_http_client.so
```

- 위 파일들을 android 프로젝트의 cpp 라이브러리 디렉토리에 복사

```powershell
\MyStelliteTest\app\src\main\cpp\stellite\bin\armeabi-v7a\libstellite_http_client.so
```

## chromium 에서 빌드된 jar 파일들 복사

```bash
build_android/src/out_android_armv7/obj/lib.java/url/url_java.jar
build_android/src/out_android_armv7/obj/lib.java/third_party/jsr-305/jsr_305_javalib.jar
build_android/src/out_android_armv7/obj/lib.java/net/android/net_java.jar
build_android/src/out_android_armv7/obj/lib.java/base/base_java.jar
```

위 파일들을 android 프로젝트의 라이브러리 디렉토리에 복사

```powershell
\MyStelliteTest\app\libs\base_java.jar
\MyStelliteTest\app\libs\jsr_305_javalib.jar
\MyStelliteTest\app\libs\net_java.jar
\MyStelliteTest\app\libs\url_java.jar
```

## NDK 빌드스크립트 작성

```powershell
\MyStelliteTest\app\build.gradle
\MyStelliteTest\app\Application.mk
\MyStelliteTest\app\Android.mk
```

## stellite 초기화 코드 작성

```powershell
\MyStelliteTest\app\src\main\java\com\naver\videotech\mystellitetest\MainActivity.java
\MyStelliteTest\app\src\main\cpp\native-lib.cpp
```

## stellite quic 요청/응답 코드 작성

```powershell
\MyStelliteTest\app\src\main\cpp\native-lib.cpp
\MyStelliteTest\app\src\main\cpp\MyStelliteHttpCallback.h
\MyStelliteTest\app\src\main\cpp\MyStelliteHttpCallback.cpp
```


## logcat으로 결과확인

```powershell
05-18 12:13:23.651 15085-15111/com.naver.videotech.mystellitetest D/MYSTELLITETEST: OnHttpResponse.OnHttpResponse request_id[1] body_len[45690] connection_info_desc[quic/1+spdy/3] connection_info[5]
    OnHttpResponse.OnHttpResponse body[<!doctype html><html itemscope="" itemtype="http://schema.org/WebPage" lang="ko"><head><meta content="text/html; charset=UTF-8" http-equiv="Content-Type"><meta content="/images/branding/googleg/1x/googleg_standard_color_128dp.png" itemprop="image"><title>Google</title><script nonce="MKu1ZruEXteRHzSTE/12Xw==">(function(){window.google={kEI:'z0T-WpS2F4S10gT_kJr4BA',kEXPI:'0,1353747,57,472,639,258,588,1018,440,184,1218,76,617,12,15,68,204,12,2340437,278,149,32,329294,1294,12383,2349,2506,32691,2075,13173,867,1580,7,1369,7,3729,5471,11344,2985,2192,368,549,466,198,2102,113,729,1472,3191,726,5,336,1375,130,130,3742,1088,2,14,261,444,131,1119,2,579,663,64,311,592,294,867,367,126,624,355,257,284,14,90,789,1412,850,154,730,1188,429,479,38,7,151,1098,8,537,311,526,196,722,45,6,189,1043,485,79,457,216,62,2,345,122,373,503,912,199,336,27,8,428,351,331,526,18,375,154,37,140,827,155,106,438,612,140,35,28,83,27,18,64,87,108,668,54,15,1,164,69,161,5,243,2,116,30,301,33,56,535,617,371,31,4

05-18 12:13:29.363 15085-15111/com.naver.videotech.mystellitetest D/MYSTELLITETEST: OnHttpResponse.OnHttpResponse request_id[2] body_len[45690] connection_info_desc[quic/1+spdy/3] connection_info[5]
    OnHttpResponse.OnHttpResponse body[<!doctype html><html itemscope="" itemtype="http://schema.org/WebPage" lang="ko"><head><meta content="text/html; charset=UTF-8" http-equiv="Content-Type"><meta content="/images/branding/googleg/1x/googleg_standard_color_128dp.png" itemprop="image"><title>Google</title><script nonce="b0QHCXPuVtSQNQ/2fLv82g==">(function(){window.google={kEI:'1UT-Wv3xB4Op0gTi2IfYBA',kEXPI:'0,1353747,57,472,639,258,588,1018,440,184,1218,76,617,12,15,68,204,12,2340437,278,149,32,329294,1294,12383,2349,2506,32691,2075,13173,867,1580,7,1369,7,3729,5471,11344,2985,2192,368,549,466,198,2102,113,729,1472,3191,726,5,336,1375,130,130,3742,1088,2,14,261,444,131,1119,2,579,663,64,311,592,294,867,367,126,624,355,257,284,14,90,789,1412,850,154,730,1188,429,479,38,7,151,1098,8,537,311,526,196,722,45,6,189,1043,485,79,457,216,62,2,345,122,373,503,912,199,336,27,8,428,351,331,526,18,375,154,37,140,827,155,106,438,612,140,35,28,83,27,18,64,87,108,668,54,15,1,164,69,161,5,243,2,116,30,301,33,56,535,617,371,31,4
```