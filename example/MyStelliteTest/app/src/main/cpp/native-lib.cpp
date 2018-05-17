#include <jni.h>
#include <string>
#include "MyStelliteHttpCallback.h"


stellite::HttpClientContext *g_pHttpClientContext = nullptr;
MyStelliteHttpCallback *g_pCallback = nullptr;
stellite::HttpClient *g_pHttpClient = nullptr;

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }

    stellite::HttpClientContext::InitVM(vm);
    return JNI_VERSION_1_6;
}


extern "C" JNIEXPORT jstring
JNICALL
Java_com_naver_videotech_mystellitetest_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {

    if (g_pHttpClientContext == nullptr) {
        stellite::HttpClientContext::Params client_params;
        client_params.using_quic = true;
        client_params.using_http2 = true;
        client_params.ignore_certificate_errors = true;
        client_params.enable_http2_alternative_service_with_different_host = true;
        client_params.enable_quic_alternative_service_with_different_host = true;
        client_params.origins_to_force_quic_on.push_back("www.google.co.kr");
        g_pHttpClientContext = new stellite::HttpClientContext(client_params);

        g_pHttpClientContext->Initialize();
        g_pCallback = new MyStelliteHttpCallback();
        g_pHttpClient = g_pHttpClientContext->CreateHttpClient(g_pCallback);
    }

    stellite::HttpRequest request;
    request.request_type = stellite::HttpRequest::RequestType::GET;
    request.url = "https://www.google.co.kr:443";
    g_pHttpClient->Request(request);

    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}
