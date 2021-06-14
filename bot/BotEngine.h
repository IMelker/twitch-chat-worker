//
// Created by imelker on 02.04.2021.
//

#ifndef CHATSNIFFER_BOT_BOTENGINE_H_
#define CHATSNIFFER_BOT_BOTENGINE_H_

#include <atomic>
#include <vector>

#include <so_5/agent.hpp>

#include "BotConfiguration.h"
#include "BotEvents.h"

class Logger;
class IRCController;

class BotMessageEvent;
class BotMessageEventHandler;

class BotEngine final : public so_5::agent_t
{
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

    const BotConfiguration& getConfig() const;
    const std::shared_ptr<Logger>& getLogger() const;
    const so_5::mbox_t& getMsgSender() const;
    const so_5::mbox_t& getBotLogger() const;

    // control event handlers
    void evtShutdown(mhood_t<Bot::Shutdown> message);
    void evtReload(mhood_t<Bot::Reload> message);

    // bot event handlers
    void evtBotMessage(so_5::mhood_t<BotMessageEvent> evt);
  private:
    void loadHandlers();

    so_5::mbox_t self;
    so_5::mbox_t msgSender;
    so_5::mbox_t botLogger;

    const std::shared_ptr<Logger> logger;

    BotConfiguration config;

    std::vector<std::unique_ptr<BotMessageEventHandler>> massageHandlers;
};

#endif //CHATSNIFFER_BOT_BOTENGINE_H_
