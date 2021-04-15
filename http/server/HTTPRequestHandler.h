//
// Created by l2pic on 15.04.2021.
//

#ifndef CHATCONTROLLER_HTTP_SERVER_HTTPREQUESTHANDLER_H_
#define CHATCONTROLLER_HTTP_SERVER_HTTPREQUESTHANDLER_H_

#include "HTTPSession.h"

struct HTTPRequestHandler
{
    virtual void handleRequest(http::request<http::string_body> &&req, HTTPSession::SendLambda &&send) = 0;
};

#endif //CHATCONTROLLER_HTTP_SERVER_HTTPREQUESTHANDLER_H_
