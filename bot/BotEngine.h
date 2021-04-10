//
// Created by imelker on 02.04.2021.
//

#ifndef CHATSNIFFER_BOT_BOTENGINE_H_
#define CHATSNIFFER_BOT_BOTENGINE_H_

#include <atomic>
#include "../Message.h"
#include "../MessageSenderInterface.h"
#include "BotConfiguration.h"
#include "BotLogger.h"

class Logger;
class IRCWorkerPool;

class BotEngine
{
  public:
    BotEngine(BotConfiguration config, std::shared_ptr<Logger> logger);
    ~BotEngine();

    void start();
    void stop();

    [[nodiscard]] int getId() const;

    void setConfig(BotConfiguration cfg);
    void setSender(MessageSenderInterface *sender);
    void setBotLogger(BotLogger *blogger);

    void handleInterval(Handler& handler);
    void handleTimer(Handler& handler);

    // event handlers
    void handleMessage(std::shared_ptr<Message> message);
  private:
    MessageSenderInterface *irc;
    BotLogger *botLogger;
    std::shared_ptr<Logger> logger;

    std::atomic_bool active;

    BotConfiguration config;
};

#endif //CHATSNIFFER_BOT_BOTENGINE_H_
