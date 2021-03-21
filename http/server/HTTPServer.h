//
// Created by l2pic on 18.03.2021.
//

#ifndef CHATSNIFFER_HTTP_HTTPSERVER_H_
#define CHATSNIFFER_HTTP_HTTPSERVER_H_

#include <memory>
#include <unordered_map>
#include <mutex>
#include <string>

#include <boost/asio.hpp>

#include "HTTPServerUnit.h"
#include "HTTPSession.h"

class Logger;
class HTTPListener;
class ThreadPool;

struct HTTPControlConfig {
    std::string host = "localhost";
    unsigned short port = 8080;
    std::string user = "admin";
    std::string pass = "admin";
    bool secure = true;
    bool verify = true;
    int threads = 1;
};

class HTTPServer : public HTTPRequestHandler, public HTTPServerUnit
{
  public:
    HTTPServer(HTTPControlConfig config, std::shared_ptr<Logger> logger);
    ~HTTPServer();

    bool start(ThreadPool *pool);
    void stop();

    void addControlUnit(const std::string& path, HTTPServerUnit *unit);
    void deleteControlUnit(const std::string& path);

    // implement HTTPRequestHandler
    void handleRequest(http::request<http::string_body> &&req, HTTPSession::SendLambda &&send) override;

    // implement HTTPControlUnit
    std::tuple<int, std::string> processHttpRequest(std::string_view path, const std::string &body, std::string &error) override;
  private:
    HTTPControlConfig config;
    std::shared_ptr<Logger> logger;

    boost::asio::io_context ioc;
    std::shared_ptr<HTTPListener> listener;
    std::vector<std::thread> ioRunners;
    ThreadPool *executors;

    std::mutex unitsMutex;
    std::map<std::string, HTTPServerUnit *, std::less<>> units;
};

#endif //CHATSNIFFER_HTTP_HTTPSERVER_H_
