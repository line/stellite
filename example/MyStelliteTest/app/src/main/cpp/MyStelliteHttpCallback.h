//
// Created by USER on 2018-05-14.
//

#ifndef MYSTELLITEHTTPCALLBACK_H
#define MYSTELLITEHTTPCALLBACK_H


#include <string>
#include "MyCommon.h"
#include "stellite/include/http_client.h"
#include "stellite/include/http_client_context.h"
#include "stellite/include/http_request.h"
#include "stellite/include/http_response.h"


class MyStelliteHttpCallback : public stellite::HttpResponseDelegate {
public:

    MyStelliteHttpCallback();

    virtual ~MyStelliteHttpCallback() {}

    void OnHttpResponse(int request_id,
                        const stellite::HttpResponse &response,
                        const char *body,
                        size_t body_len);

    void OnHttpStream(int request_id,
                      const stellite::HttpResponse &response,
                      const char *stream,
                      size_t stream_len,
                      bool is_last);

    // The error code are defined at net/base/net_error_list.h
    void OnHttpError(int request_id,
                     int error_code,
                     const std::string &error_message);

private:
};

#endif //MYSTELLITEHTTPCALLBACK_H
