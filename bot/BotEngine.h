//
// Created by imelker on 02.04.2021.
//

#ifndef CHATSNIFFER_BOT_BOTENGINE_H_
#define CHATSNIFFER_BOT_BOTENGINE_H_

#include <atomic>

#include <so_5/agent.hpp>

#include "../ChatMessage.h"
#include "BotConfiguration.h"
#include "BotLogger.h"

class Logger;
class IRCController;

class BotEngine final : public so_5::agent_t
{
  public:
    struct Shutdown final : public so_5::signal_t {};
    struct Reload { mutable BotConfiguration config; };
  public:
    BotEngine(const context_t &ctx,
              so_5::mbox_t self,
              so_5::mbox_t msgSender,
              so_5::mbox_t botLogger,
              BotConfiguration config,
              std::shared_ptr<Logger> logger);
    ~BotEngine() override;

    void so_define_agent() override;
    void so_evt_start() override;
    void so_evt_finish() override;

    // event handlers
    void evtShutdown(mhood_t<Shutdown> message);
    void evtReload(mhood_t<Reload> message);
    void evtChatMessage(mhood_t<Chat::Message> message);

    void handleInterval(Handler& handler);
    void handleTimer(Handler& handler);
  private:
    so_5::mbox_t self;
    so_5::mbox_t msgSender;
    so_5::mbox_t botLogger;

    std::shared_ptr<Logger> logger;

    BotConfiguration config;
};

#endif //CHATSNIFFER_BOT_BOTENGINE_H_
