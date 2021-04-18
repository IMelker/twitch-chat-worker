//
// Created by imelker on 18.03.2021.
//

#ifndef CHATSNIFFER_HTTP_HTTPSERVER_H_
#define CHATSNIFFER_HTTP_HTTPSERVER_H_

#include <memory>
#include <unordered_map>
#include <shared_mutex>
#include <string>

#include <boost/asio.hpp>
#include <boost/beast/ssl.hpp>

#include "HTTPRequestHandler.h"
#include "HTTPSession.h"

class Logger;
class HTTPListener;

struct HTTPControlConfig {
    std::string host = "localhost";
    unsigned short port = 8080;
    std::string user;
    std::string pass;
    bool secure = true;
    struct {
        bool verify = false;
        std::string cert;
        std::string key;
        std::string dh;
    } ssl;
    unsigned int threads = 1;
};

class HTTPServer : public HTTPRequestHandler
{
  public:
    HTTPServer(HTTPControlConfig config, HTTPRequestHandler *handler, std::shared_ptr<Logger> logger);
    ~HTTPServer();

    bool start();
    void stop();

    // implement HTTPRequestHandler
    void handleRequest(http::request<http::string_body> &&req, HTTPSession::SendLambda &&send) override;
  protected:
    HTTPControlConfig config;
    std::shared_ptr<Logger> logger;

    std::string basicAuth;

    boost::asio::io_context ioc;
    boost::asio::ssl::context ctx;
    std::shared_ptr<HTTPListener> listener;
    std::vector<std::thread> ioRunners;

    HTTPRequestHandler *handler;
};

#endif //CHATSNIFFER_HTTP_HTTPSERVER_H_
