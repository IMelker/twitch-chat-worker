//
// Created by l2pic on 02.04.2021.
//

#ifndef CHATSNIFFER_BOT_BOTENVIRONMENT_H_
#define CHATSNIFFER_BOT_BOTENVIRONMENT_H_

#include <memory>
#include <string>
#include <map>

#include "../http/server/HTTPServerUnit.h"
#include "../MessageSubscriber.h"
#include "BotEngine.h"

class Logger;
class BotsEnvironment : public MessageSubscriber, public HTTPServerUnit
{
  public:
    BotsEnvironment(std::shared_ptr<Logger> logger);
    ~BotsEnvironment();

    // implement MessageSubscriber
    void onMessage(const Message &message) override;

    // implement HTTPControlUnit:
    std::tuple<int, std::string> processHttpRequest(std::string_view path, const std::string &request,
                                                    std::string &error) override;
  private:
    std::shared_ptr<Logger> logger;
    std::map<std::string, std::shared_ptr<BotEngine>> bots;
};

#endif //CHATSNIFFER_BOT_BOTENVIRONMENT_H_
