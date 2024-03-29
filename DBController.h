//
// Created by imelker on 03.04.2021.
//

#ifndef CHATSNIFFER__DBCONTROLLER_H_
#define CHATSNIFFER__DBCONTROLLER_H_

#include <memory>
#include <string>
#include <map>
#include <vector>
#include <unordered_set>

#include "db/pg/PGConnectionPool.h"
#include "bot/BotConfiguration.h"
#include "irc/IRCClientConfig.h"

class Logger;
class DBController
{
  public:
    using Channels = std::vector<std::string>;
    using UpdatedChannels = std::map<std::string, bool>;
    using Users = std::unordered_set<std::string>;
    using Account = IRCClientConfig;
    using Accounts = std::vector<Account>;
    using BotsConfigurations = std::map<int, BotConfiguration>;
  public:
    DBController(PGConnectionConfig config, unsigned int count, std::shared_ptr<Logger> logger);
    ~DBController();

    [[nodiscard]] std::string getStats() const;
    [[nodiscard]] PGConnectionPool *getPGPool() const;

    Users loadUsersNicknames();
    Users loadServiceAccountsNicknames();
    Accounts loadAccounts();
    Account loadAccount(int id);
    Channels loadChannels();
    Channels loadChannelsFor(const std::string& user);
    Channels loadChannelsFor(int accountId);
    UpdatedChannels loadChannels(long long &timestamp);
    BotsConfigurations loadBotConfigurations();
    BotConfiguration loadBotConfiguration(int id);
  private:
    const std::shared_ptr<Logger> logger;
    std::shared_ptr<PGConnectionPool> pg;
};

#endif //CHATSNIFFER__DBCONTROLLER_H_
