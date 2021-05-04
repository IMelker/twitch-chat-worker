//
// Created by imelker on 06.03.2021.
//

#ifndef CHATSNIFFER__CONTROLLER_H_
#define CHATSNIFFER__CONTROLLER_H_

#include <map>
#include <string>
#include <vector>

#include <so_5/agent.hpp>
#include <so_5/environment.hpp>

#include "Config.h"

#include "http/server/HTTPServer.h"
#include "db/pg/PGConnectionPool.h"
#include "irc/IRCController.h"
#include "bot/BotsEnvironment.h"
#include "db/ch/CHConnectionPool.h"
#include "MessageProcessor.h"
#include "DBController.h"
#include "Storage.h"

class Logger;
class Controller final : public so_5::agent_t
{
    struct ShutdownCheck final : public so_5::signal_t {};

  public:
    explicit Controller(const context_t& ctx,
                        so_5::mbox_t http,
                        Config &config,
                        std::shared_ptr<DBController> db,
                        std::shared_ptr<Logger> logger);
    ~Controller() override;

    void so_define_agent() override;
    void so_evt_start() override;
    void so_evt_finish() override;

    Storage *makeStorage(so_5::coop_t &coop, const so_5::mbox_t& listener);
    BotsEnvironment *makeBotsEnvironment(so_5::coop_t &coop, const so_5::mbox_t& listener);
    MessageProcessor *makeMessageProcessor(so_5::coop_t &coop, const so_5::mbox_t& publisher);
    IRCController *makeIRCController(so_5::coop_t &coop);

  private:
    Config &config;

    const std::shared_ptr<Logger> logger;
    const std::shared_ptr<DBController> db;

    Storage *storage = nullptr;
    BotsEnvironment *botsEnvironment = nullptr;
    MessageProcessor *msgProcessor = nullptr;
    IRCController *ircController = nullptr;

    so_5::mbox_t http;
    so_5::timer_id_t shutdownCheckTimer;
};

#endif //CHATSNIFFER__CONTROLLER_H_
