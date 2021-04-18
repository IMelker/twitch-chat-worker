//
// Created by imelker on 03.04.2021.
//

#ifndef CHATSNIFFER__DBCONTROLLER_H_
#define CHATSNIFFER__DBCONTROLLER_H_

#include <memory>
#include <vector>
#include <string>
#include <map>
#include "IRCWorker.h"
#include "db/pg/PGConnectionPool.h"
#include "bot/BotConfiguration.h"

class Logger;
class DBController
{
  public:
    using Channels = std::vector<std::string>;
    using UpdatedChannels = std::map<std::string, bool>;
    using Users = std::vector<std::string>;
    using Accounts = std::vector<IRCClientConfig>;
    using BotsConfigurations = std::map<int, BotConfiguration>;
  public:
    DBController(PGConnectionConfig config, unsigned int count, std::shared_ptr<Logger> logger);
    ~DBController();

    std::string getStats() const;
    [[nodiscard]] PGConnectionPool *getPGPool() const;

    Users loadUsersNicknames();
    Accounts loadAccounts();
    Channels loadChannels();
    Channels loadChannelsFor(const std::string& user);
    UpdatedChannels loadChannels(long long &timestamp);
    BotsConfigurations loadBotConfigurations();
    BotConfiguration loadBotConfiguration(int id);
  private:
    std::shared_ptr<Logger> logger;
    std::shared_ptr<PGConnectionPool> pg;
};

#endif //CHATSNIFFER__DBCONTROLLER_H_
