//
// Created by l2pic on 13.04.2021.
//

#ifndef CHATCONTROLLER_HTTP_HTTPSERVERAGENT_H_
#define CHATCONTROLLER_HTTP_HTTPSERVERAGENT_H_

#include <so_5/agent.hpp>

#include "server/HTTPServer.h"

class Logger;
class HTTPServerAgent final : public so_5::agent_t, public HTTPServer
{
  public:
    HTTPServerAgent(const context_t &ctx,
                    HTTPControlConfig config,
                    std::shared_ptr<Logger> logger);
    ~HTTPServerAgent() override;

    // so_5::agent_t implementaton
    void so_define_agent() override {};
    void so_evt_start() override {};
    void so_evt_finish() override {};

    // implement HTTPRequestHandler
    void handleRequest(http::request<http::string_body> &&req, HTTPSession::SendLambda &&send) override;
  private:

};

#endif //CHATCONTROLLER_HTTP_HTTPSERVERAGENT_H_
