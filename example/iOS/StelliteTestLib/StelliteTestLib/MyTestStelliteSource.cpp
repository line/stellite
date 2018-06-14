//
//  MyTestStelliteSource.cpp
//  StelliteTest
//
//  Created by Naver on 2018. 6. 11..
//  Copyright © 2018년 Naver. All rights reserved.
//

#include "MyTestStelliteSource.h"

MyTestStelliteSource::MyTestStelliteSource() {
    
}

MyTestStelliteSource::~MyTestStelliteSource() {
    
}

void MyTestStelliteSource::request() {
    if (httpClientContext == nullptr) {
        stellite::HttpClientContext::Params client_params;
        client_params.using_quic = true;
        client_params.using_http2 = true;
        client_params.ignore_certificate_errors = true;
        client_params.enable_http2_alternative_service_with_different_host = true;
        client_params.enable_quic_alternative_service_with_different_host = true;
        client_params.origins_to_force_quic_on.push_back("www.google.co.kr");
        httpClientContext = new stellite::HttpClientContext(client_params);
        
        httpClientContext->Initialize();
        stelliteCallback = new StelliteHttpCallback();
        httpClient = httpClientContext->CreateHttpClient(stelliteCallback);
    }
    
    stellite::HttpRequest request;
    request.request_type = stellite::HttpRequest::RequestType::GET;
    request.url = "https://www.google.co.kr:443";
    httpClient->Request(request);
    
    std::string hello = "Hello from C++";
}
