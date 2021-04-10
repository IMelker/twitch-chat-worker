//
// Created by imelker on 06.03.2021.
//

#ifndef CHATSNIFFER__CONTROLLER_H_
#define CHATSNIFFER__CONTROLLER_H_

#include <map>
#include <string>
#include <vector>

#include "common/ThreadPool.h"

#include "http/server/HTTPServer.h"
#include "irc/IRCWorker.h"
#include "irc/IRCWorkerPool.h"
#include "db/pg/PGConnectionPool.h"
#include "db/ch/CHConnectionPool.h"
#include "bot/BotsEnvironment.h"
#include "MessageProcessor.h"
#include "Storage.h"
#include "DBController.h"
#include "common/Config.h"

class Logger;
class Controller : public HTTPServerUnit
{
  public:
    explicit Controller(Config &config, ThreadPool *pool, std::shared_ptr<Logger> logger);
    ~Controller();

    bool startHttpServer();
    bool startBotsEnvironment();

    bool initDBController();
    bool initIRCWorkerPool();
    bool initMessageProcessor();
    bool initMessageStorage();
    bool initBotsEnvironment();

    static void loop();

    // implement HTTPControlUnit
    std::tuple<int, std::string> processHttpRequest(std::string_view path, const std::string &request,
                                                    std::string &error) override;

    // request handlers
    std::string handleShutdown(const std::string &request, std::string &error);
    std::string handleVersion(const std::string &request, std::string &error);
  private:
    ThreadPool *pool;
    Config &config;

    std::shared_ptr<Logger> logger;
    std::shared_ptr<HTTPServer> httpServer;
    std::shared_ptr<DBController> db;

    std::shared_ptr<BotsEnvironment> botsEnvironment;
    std::shared_ptr<Storage> storage;
    std::shared_ptr<MessageProcessor> msgProcessor;
    std::shared_ptr<IRCWorkerPool> ircWorkers;
};

#endif //CHATSNIFFER__CONTROLLER_H_
