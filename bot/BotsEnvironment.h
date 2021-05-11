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
#include <so_5/coop_handle.hpp>
#include <so_5/disp/adv_thread_pool/pub.hpp>

#include "../ChatMessage.h"
#include "../HttpControllerEvents.h"
#include "BotConfiguration.h"

class Logger;
class DBController;
class BotEngine;
class BotsEnvironment final : public so_5::agent_t
{
  public:
    BotsEnvironment(const context_t &ctx,
                    so_5::mbox_t publisher,
                    so_5::mbox_t http,
                    unsigned int threads,
                    std::shared_ptr<DBController> db,
                    std::shared_ptr<Logger> logger);
    ~BotsEnvironment() override;

    void setMessageSender(so_5::mbox_t sender);
    void setBotLogger(so_5::mbox_t blogger);

    // so_5::agent_t implementation
    void so_define_agent() override;
    void so_evt_start() override;
    void so_evt_finish() override;

    void evtChatMessage(mhood_t<Chat::Message> msg);

    // http events
    void evtHttpAdd(mhood_t<hreq::bot::add> evt);
    void evtHttpRemove(mhood_t<hreq::bot::remove> evt);
    void evtHttpReload(mhood_t<hreq::bot::reload> evt);
    void evtHttpReloadAll(mhood_t<hreq::bot::reloadall> evt);
  private:
    void addBot(const BotConfiguration &config);

    so_5::mbox_t publisher;
    so_5::mbox_t msgSender;
    so_5::mbox_t botLogger;
    so_5::mbox_t http;
    so_5::coop_handle_t coop;

    so_5::disp::adv_thread_pool::dispatcher_handle_t botEnginePool;
    so_5::disp::adv_thread_pool::bind_params_t botEnginePoolParams;

    const std::shared_ptr<DBController> db;
    const std::shared_ptr<Logger> logger;

    unsigned int threads;

    std::map<int, BotEngine *> botsById;
    std::map<std::string, so_5::mbox_t> botBoxes;
    std::vector<std::string> ignoreUsers;
};

#endif //CHATSNIFFER_BOT_BOTENVIRONMENT_H_
