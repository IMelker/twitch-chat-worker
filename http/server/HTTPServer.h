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

#include "HTTPServerUnit.h"
#include "HTTPSession.h"

class Logger;
class HTTPListener;
class ThreadPool;

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
    int threads = 1;
};

class HTTPServer : public HTTPRequestHandler
{
  public:
    HTTPServer(HTTPControlConfig config, std::shared_ptr<Logger> logger);
    ~HTTPServer();

    bool start(ThreadPool *pool);
    void clearUnits();

    void addControlUnit(const std::string& path, HTTPServerUnit *unit);
    void deleteControlUnit(const std::string& path);

    // implement HTTPRequestHandler
    void handleRequest(http::request<http::string_body> &&req, HTTPSession::SendLambda &&send) override;
  protected:
    HTTPControlConfig config;
    std::shared_ptr<Logger> logger;

    boost::asio::io_context ioc;
    boost::asio::ssl::context ctx;
    std::shared_ptr<HTTPListener> listener;
    std::vector<std::thread> ioRunners;
    ThreadPool *executors;

    std::shared_mutex unitsMutex;
    std::map<std::string, HTTPServerUnit *, std::less<>> units;
};

#endif //CHATSNIFFER_HTTP_HTTPSERVER_H_
