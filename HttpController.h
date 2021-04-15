//
// Created by l2pic on 15.04.2021.
//

#ifndef CHATCONTROLLER__HTTPCONTROLLER_H_
#define CHATCONTROLLER__HTTPCONTROLLER_H_

#include <so_5/agent.hpp>

#include "common/Config.h"
#include "http/server/HTTPServer.h"

class Logger;
class HttpController final : public so_5::agent_t
{
    struct ShutdownCheck final : public so_5::signal_t {};

  public:
    HttpController(const context_t &ctx, Config &config, std::shared_ptr<Logger> logger);
    ~HttpController() override;

    // so_5::agent_t implementation
    void so_define_agent() override;
    void so_evt_start() override;
    void so_evt_finish() override;

  private:
    Config &config;

    std::shared_ptr<Logger> logger;
    std::shared_ptr<HTTPServer> server;
};

#endif //CHATCONTROLLER__HTTPCONTROLLER_H_
