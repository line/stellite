// whattotest.cpp
#include <future>
#include <gtest/gtest.h>
#include "MyStelliteHttpCallback.h"
#include "stellite/include/http_client.h"
#include "stellite/include/http_client_context.h"
#include "stellite/include/http_request.h"
#include "stellite/include/http_response.h"

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

stellite::HttpClientContext *g_pHttpClientContext = nullptr;
MyStelliteHttpCallback *g_pCallback = nullptr;
stellite::HttpClient *g_pHttpClient = nullptr;

TEST(stellite, get_quic)
{
    std::promise<void> prom;

    if (g_pHttpClientContext == nullptr)
    {
        stellite::HttpClientContext::Params client_params;
        client_params.using_quic = true;
        client_params.using_http2 = true;
        client_params.ignore_certificate_errors = true;
        client_params.enable_http2_alternative_service_with_different_host = true;
        client_params.enable_quic_alternative_service_with_different_host = true;
        client_params.origins_to_force_quic_on.push_back("www.google.co.kr");
        g_pHttpClientContext = new stellite::HttpClientContext(client_params);

        g_pHttpClientContext->Initialize();
        g_pCallback = new MyStelliteHttpCallback(&prom);
        g_pHttpClient = g_pHttpClientContext->CreateHttpClient(g_pCallback);
    }

    stellite::HttpRequest request;
    request.request_type = stellite::HttpRequest::RequestType::GET;
    request.url = "https://www.google.co.kr:443";
    printf("request.url[%s] \n", request.url);
    g_pHttpClient->Request(request);
    auto fut = prom.get_future();
    fut.wait();
}