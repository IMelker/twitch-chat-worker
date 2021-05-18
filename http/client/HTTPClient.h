//
// Created by l2pic on 15.04.2021.
//

#ifndef CHATCONTROLLER_HTTP_CLIENT_HTTPCLIENT_H_
#define CHATCONTROLLER_HTTP_CLIENT_HTTPCLIENT_H_

#include <string>

#include <boost/asio.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/thread/thread.hpp>

#include "HTTPRequest.h"

struct HTTPClientConfig {
    int version = 11;
    std::string userAgent = "HTTPClient";
    unsigned int timeout = 30;
    bool secure = true;
    struct {
        bool verify = false;
        std::string cert;
        std::string key;
        std::string dh;
    } ssl;
    unsigned int threads = 1;
};

class Logger;
class HTTPClient
{
  public:
    HTTPClient(HTTPClientConfig config, std::shared_ptr<Logger> logger);
    ~HTTPClient();

    bool start();
    void stop();

    void exec(HTTPRequest &&request);

  private:
    HTTPClientConfig config;
    std::shared_ptr<Logger> logger;

    boost::asio::ssl::context ctx;
    boost::asio::io_service ioService;
    boost::thread_group threadPool;
    boost::asio::io_service::work work;
};

#endif //CHATCONTROLLER_HTTP_CLIENT_HTTPCLIENT_H_
