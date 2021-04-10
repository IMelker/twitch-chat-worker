//
// Created by imelker on 02.04.2021.
//

#ifndef CHATSNIFFER_BOT_BOTENVIRONMENT_H_
#define CHATSNIFFER_BOT_BOTENVIRONMENT_H_

#include <shared_mutex>
#include <memory>
#include <string>
#include <map>

#include "BotLogger.h"
#include "../http/server/HTTPServerUnit.h"
#include "../MessageSubscriber.h"
#include "../MessageSenderInterface.h"

class Logger;
class ThreadPool;
class DBController;
class BotEngine;

class BotsEnvironment : public MessageSubscriber, public HTTPServerUnit
{
  public:
    BotsEnvironment(ThreadPool *pool, DBController *db, std::shared_ptr<Logger> logger);
    ~BotsEnvironment();

    void setSender(MessageSenderInterface *sender);
    void setBotLogger(BotLogger *blogger);

    void start();
    void stop();

    // implement MessageSubscriber
    void onMessage(std::shared_ptr<Message> message) override;

    // implement HTTPControlUnit:
    std::tuple<int, std::string> processHttpRequest(std::string_view path, const std::string &request,
                                                    std::string &error) override;
    // http handlers
    std::string handleReloadConfiguration(const std::string &request, std::string &error);
  private:
    std::shared_ptr<BotEngine> addBot(BotConfiguration config);

    ThreadPool *pool;
    DBController *db;
    MessageSenderInterface *irc = nullptr;
    BotLogger *botLogger = nullptr;
    std::shared_ptr<Logger> logger;

    std::shared_mutex botsMutex;
    std::map<int, std::shared_ptr<BotEngine>> botsById;
    std::multimap<std::string, std::shared_ptr<BotEngine>> botsByChannel;
};

#endif //CHATSNIFFER_BOT_BOTENVIRONMENT_H_
