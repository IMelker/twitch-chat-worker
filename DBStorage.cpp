//
// Created by imelker on 03.04.2021.
//

#include "common/Utils.h"
#include "common/Logger.h"
#include "irc/IRCWorker.h"
#include "db/DBConnectionLock.h"
#include "DBStorage.h"

DBStorage::DBStorage(PGConnectionConfig config, unsigned int count, std::shared_ptr<Logger> logger)
: logger(std::move(logger)) {
    this->logger->logInfo("DBStorage init PostgreSQL DB pool with {} connections", count);
    pg = std::make_shared<PGConnectionPool>(std::move(config), count, this->logger);
}

DBStorage::~DBStorage() {
    logger->logTrace("DBStorage end of storage");
}

PGConnectionPool *DBStorage::getPGPool() const {
    return pg.get();
}

std::vector<IRCClientConfig> DBStorage::loadAccounts() {
    static const std::string request = "SELECT username, display, oauth, channels_limit, whisper_per_sec_limit,"
                                       "auth_per_sec_limit, command_per_sec_limit "
                                       "FROM accounts "
                                       "WHERE active IS TRUE;";

    std::vector<IRCClientConfig> accounts;
    {
        DBConnectionLock dbl(pg);
        if (!dbl->ping())
            return accounts;

        std::vector<std::vector<std::string>> res;
        if (dbl->request(request, res)) {
            accounts.reserve(res.size());
            for (auto &row: res) {
                IRCClientConfig config;
                config.nick = std::move(row[0]);
                config.user = std::move(row[1]);
                config.password = std::move(row[2]);
                config.channels_limit = Utils::String::toNumber(row.at(3));
                config.whisper_per_sec_limit = Utils::String::toNumber(row.at(4));
                config.auth_per_sec_limit = Utils::String::toNumber(row.at(5));
                config.command_per_sec_limit = Utils::String::toNumber(row.at(6));

                accounts.push_back(std::move(config));
            }
        }
    }

    DefaultLogger::logInfo("Controller {} accounts loaded", accounts.size());
    return accounts;
}


std::vector<std::string> DBStorage::loadChannels() {
    static const std::string request = "SELECT name FROM channel WHERE watch IS TRUE;";

    std::vector<std::string> channels;
    {
        DBConnectionLock dbl(pg);
        if (!dbl->ping())
            return channels;

        std::vector<std::vector<std::string>> res;
        if (dbl->request(request, res)) {
            channels.reserve(res.size());
            std::transform(res.begin(), res.end(), std::back_inserter(channels),
                           [](auto& row) -> std::string&& { return std::move(row[0]); });
        }
    }

    DefaultLogger::logInfo("Controller {} channels loaded", channels.size());
    return channels;
}

std::map<int, BotConfiguration> DBStorage::loadBotConfigurations() {
    static const std::string request = "SELECT bot.id, channel.name, eh.event_type_id, eh.script "
                                       "FROM bot "
                                       "JOIN bot_channel ON bot.id = bot_channel.bot_id "
                                       "JOIN channel ON bot_channel.channel_id = channel.id "
                                       "JOIN event_handler as eh on bot.id = eh.bot_id;";

    std::map<int, BotConfiguration> configurations;
    {
        DBConnectionLock dbl(pg);
        if (!dbl->ping())
            return configurations;

        std::vector<std::vector<std::string>> res;
        if (dbl->request(request, res)) {
            using namespace Utils::String;
            for(auto &row : res) {
                int id = toNumber(row[0]);
                auto &config = configurations[id];
                config.botId = id;
                config.channel = std::move(row[1]);
                auto &handlers = getHandlers(toNumber(row[2]), config);
                handlers.push_back(std::move(row[3]));
            }
        }
    }

    DefaultLogger::logInfo("Controller {} bot configurations loaded", configurations.size());
    return configurations;
}
