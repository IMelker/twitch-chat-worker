//
// Created by imelker on 02.04.2021.
//

#ifndef CHATSNIFFER_BOT_BOTENGINE_H_
#define CHATSNIFFER_BOT_BOTENGINE_H_

#include "../Message.h"
#include "BotConfiguration.h"

class Logger;
class IRCWorkerPool;

class BotEngine
{
  public:
    explicit BotEngine(BotConfiguration config, IRCWorkerPool *irc, std::shared_ptr<Logger> logger);
    ~BotEngine();

    int getId() const;

    void updateConfiguration(BotConfiguration config);

    // event handlers
    void handleMessage(const Message &message);
  private:
    IRCWorkerPool *irc;
    std::shared_ptr<Logger> logger;

    BotConfiguration config;
};

#endif //CHATSNIFFER_BOT_BOTENGINE_H_
