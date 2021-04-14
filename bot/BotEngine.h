//
// Created by imelker on 02.04.2021.
//

#ifndef CHATSNIFFER_BOT_BOTENGINE_H_
#define CHATSNIFFER_BOT_BOTENGINE_H_

#include <atomic>

#include <so_5/agent.hpp>

#include "../ChatMessage.h"
#include "../MessageSenderInterface.h"
#include "BotConfiguration.h"
#include "BotLogger.h"

class Logger;
class IRCWorkerPool;

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

    void handleMessage(mhood_t<Chat::Message> message);

    void start();
    void stop();

    [[nodiscard]] int getId() const;

    void setConfig(BotConfiguration cfg);

    void handleInterval(Handler& handler);
    void handleTimer(Handler& handler);

    // event handlers

  private:
    so_5::mbox_t self;
    so_5::mbox_t msgSender;
    so_5::mbox_t botLogger;

    std::shared_ptr<Logger> logger;

    std::atomic_bool active;

    BotConfiguration config;
};

#endif //CHATSNIFFER_BOT_BOTENGINE_H_
