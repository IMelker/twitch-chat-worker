//
// Created by l2pic on 15.04.2021.
//

#ifndef CHATCONTROLLER__HTTPCONTROLLER_H_
#define CHATCONTROLLER__HTTPCONTROLLER_H_

#include <so_5/agent.hpp>
#include <so_5/timers.hpp>

#include "Config.h"

#include "HttpControllerEvents.h"
#include "http/server/HTTPServer.h"
#include "http/server/HTTPRequestHandler.h"

class Logger;
class DBController;
class HttpController final : public so_5::agent_t, public HTTPRequestHandler
{
    struct ShutdownCheck final : public so_5::signal_t {};

  public:
    HttpController(const context_t &ctx,
                   so_5::mbox_t listeners,
                   Config &config,
                   std::shared_ptr<Logger> logger);
    ~HttpController() override;

    // so_5::agent_t implementation
    void so_define_agent() override;
    void so_evt_start() override;
    void so_evt_finish() override;

    // implement HTTPRequestHandler
    void handleRequest(http::request<http::string_body> &&req, HTTPSession::SendLambda &&send) override;
  private:
    Config &config;

    const std::shared_ptr<Logger> logger;
    std::shared_ptr<HTTPServer> server;

    so_5::mbox_t listeners;

    so_5::timer_id_t shutdownCheckTimer;
};

#endif //CHATCONTROLLER__HTTPCONTROLLER_H_
