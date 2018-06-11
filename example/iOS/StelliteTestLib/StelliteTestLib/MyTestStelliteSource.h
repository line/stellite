//
//  MyTestStelliteSource.h
//  StelliteTest
//
//  Created by Naver on 2018. 6. 11..
//  Copyright © 2018년 Naver. All rights reserved.
//

#ifndef MyTestStelliteSource_h
#define MyTestStelliteSource_h

#include "stellite/http_client.h"
#include "stellite/http_client_context.h"
#include "stellite/http_request.h"
#include "stellite/http_response.h"
#include "StelliteHttpCallback.h"

class MyTestStelliteSource {
public:
    MyTestStelliteSource();
    virtual ~MyTestStelliteSource();

    void request();
    
private:
    stellite::HttpClientContext *httpClientContext = nullptr;
    StelliteHttpCallback *stelliteCallback = nullptr;
    stellite::HttpClient *httpClient = nullptr;
};

#endif /* MyTestStelliteSource_h */
