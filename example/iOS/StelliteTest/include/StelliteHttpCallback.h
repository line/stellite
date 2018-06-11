//
//  StelliteHttpCallback.h
//  StelliteTest
//
//  Created by Naver on 2018. 6. 11..
//  Copyright © 2018년 Naver. All rights reserved.
//

#ifndef StelliteHttpCallback_h
#define StelliteHttpCallback_h

#include "http_client.h"
#include "http_client_context.h"
#include "http_request.h"
#include "http_response.h"

class StelliteHttpCallback: public stellite::HttpResponseDelegate {
public:
    StelliteHttpCallback();
    virtual ~StelliteHttpCallback();
    
    void OnHttpResponse(int request_id,
                        const stellite::HttpResponse &response,
                        const char *body,
                        size_t body_len);
    
    void OnHttpStream(int request_id,
                      const stellite::HttpResponse &response,
                      const char *stream,
                      size_t stream_len,
                      bool is_last);
    
    void OnHttpError(int request_id,
                     int error_code,
                     const std::string &error_message);
    
};

#endif /* StelliteHttpCallback_h */
