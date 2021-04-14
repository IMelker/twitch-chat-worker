//
// Created by imelker on 02.04.2021.
//

#ifndef CHATSNIFFER_BOT_BOTENVIRONMENT_H_
#define CHATSNIFFER_BOT_BOTENVIRONMENT_H_

#include <shared_mutex>
#include <memory>
#include <string>
#include <map>

#include <so_5/agent.hpp>

#include "BotLogger.h"
#include "../ChatMessage.h"
#include "../http/server/HTTPServerUnit.h"
#include "../MessageSenderInterface.h"

class Logger;
class ThreadPool;
class DBController;
class BotEngine;

class BotsEnvironment final : public so_5::agent_t, public HTTPServerUnit
{
  public:
    BotsEnvironment(const context_t &ctx,
                    so_5::mbox_t publisher,
                    DBController *db,
                    std::shared_ptr<Logger> logger);
    ~BotsEnvironment() override;

    void setMessageSender(so_5::mbox_t sender);
    void setBotLogger(so_5::mbox_t blogger);

    // so_5::agent_t implementation
    void so_define_agent() override;
    void so_evt_start() override;
    void so_evt_finish() override;

    void evtChatMessage(mhood_t<Chat::Message> msg);

    // implement HTTPControlUnit:
    std::tuple<int, std::string> processHttpRequest(std::string_view path, const std::string &request,
                                                    std::string &error) override;
    // http handlers
    std::string handleReloadConfiguration(const std::string &request, std::string &error);
  private:
    so_5::mbox_t publisher;
    so_5::mbox_t msgSender;
    so_5::mbox_t botLogger;

    DBController *db;
    std::shared_ptr<Logger> logger;

    std::map<int, BotEngine *> botsById;
    std::map<std::string, so_5::mbox_t> botBoxes;
    std::vector<std::string> ignoreUsers;
};

#endif //CHATSNIFFER_BOT_BOTENVIRONMENT_H_
