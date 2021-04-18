//
// Created by imelker on 03.04.2021.
//

#include <fmt/format.h>

#include "common/Utils.h"
#include "common/Logger.h"
#include "common/Clock.h"
#include "IRCWorker.h"
#include "db/DBConnectionLock.h"
#include "DBController.h"

DBController::DBController(PGConnectionConfig config, unsigned int count, std::shared_ptr<Logger> logger)
: logger(std::move(logger)) {
    this->logger->logInfo("DBController init PostgreSQL DB pool with {} connections", count);
    pg = std::make_shared<PGConnectionPool>(std::move(config), count, this->logger);
}

DBController::~DBController() {
    logger->logTrace("DBController end of storage");
}

std::string DBController::getStats() const {
    return pg->statsDump();
}

PGConnectionPool *DBController::getPGPool() const {
    return pg.get();
}

DBController::Users DBController::loadUsersNicknames() {
    static const std::string request = "SELECT username FROM accounts WHERE active = true;";

    DBController::Users users;
    {
        DBConnectionLock dbl(pg);
        if (!dbl->ping())
            return users;

        std::vector<std::vector<std::string>> res;
        if (dbl->request(request, res)) {
            users.reserve(res.size());
            std::transform(res.begin(), res.end(), std::back_inserter(users), [] (auto& row) -> std::string&& {
                return std::move(row[0]);
            });
        }
    }

    DefaultLogger::logInfo("Controller {} our users loaded", users.size());
    return users;
}

DBController::Accounts DBController::loadAccounts() {
    static const std::string request = "SELECT username, display, oauth, channels_limit, "
                                       "whisper_per_sec_limit, auth_per_sec_limit, "
                                       "command_per_sec_limit FROM accounts WHERE active = true;";

    DBController::Accounts accounts;
    {
        DBConnectionLock dbl(pg);
        if (!dbl->ping())
            return accounts;

        std::vector<std::vector<std::string>> res;
        if (dbl->request(request, res)) {
            for (auto &row : res) {
                using Utils::String::toNumber;
                IRCClientConfig cfg;
                cfg.nick = std::move(row[0]);
                cfg.user = std::move(row[1]);
                cfg.password = std::move(row[2]);
                cfg.channels_limit = toNumber(row[3]);
                cfg.command_per_sec_limit = toNumber(row[4]);
                cfg.whisper_per_sec_limit = toNumber(row[5]);
                cfg.auth_per_sec_limit = toNumber(row[6]);
                accounts.push_back(std::move(cfg));
            }
        }
    }

    DefaultLogger::logInfo("Controller {} accounts loaded", accounts.size());
    return accounts;
}

DBController::Channels DBController::loadChannels() {
    std::string request = "SELECT name FROM channel WHERE watch = true;";

    DBController::Channels channels;
    {
        DBConnectionLock dbl(pg);
        if (!dbl->ping())
            return channels;

        std::vector<std::vector<std::string>> res;
        if (dbl->request(request, res)) {
            std::transform(res.begin(), res.end(), std::back_inserter(channels),
                           [] (auto &row) -> std::string && { return std::move(row[0]); });
        }
    }

    DefaultLogger::logInfo("Controller {} watched channels loaded", channels.size());
    return channels;
}

DBController::Channels DBController::loadChannelsFor(const std::string &user) {
    std::string request = fmt::format("SELECT channel.name FROM bot_account "
                                      "JOIN bot_channel ON bot_account.bot_id = bot_channel.bot_id "
                                      "JOIN accounts ON bot_account.account_id = accounts.id "
                                      "JOIN channel ON bot_channel.channel_id = channel.id "
                                      "WHERE accounts.username = '{}';", user);

    DBController::Channels channels;
    {
        DBConnectionLock dbl(pg);
        if (!dbl->ping())
            return channels;

        std::vector<std::vector<std::string>> res;
        if (dbl->request(request, res)) {
            std::transform(res.begin(), res.end(), std::back_inserter(channels),
                [] (auto &row) -> std::string && { return std::move(row[0]); });
        }
    }

    DefaultLogger::logInfo("Controller {} channels loaded for {}", channels.size(), user);
    return channels;
}


DBController::BotsConfigurations DBController::loadBotConfigurations() {
    static const std::string request = "SELECT bot.id, bot.user_id, accounts.username, "
                                       "       channel.name, eh.event_type_id, "
                                       "       eh.script, eh.id, eh.additional "
                                       "FROM bot "
                                       "JOIN bot_account ON bot.id = bot_account.bot_id "
                                       "JOIN accounts ON bot_account.account_id = accounts.id "
                                       "JOIN bot_channel ON bot.id = bot_channel.bot_id "
                                       "JOIN channel ON bot_channel.channel_id = channel.id "
                                       "JOIN event_handler as eh on bot.id = eh.bot_id;";

    DBController::BotsConfigurations configurations;
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
                config.userId = toNumber(row[1]);
                config.account = std::move(row[2]);
                config.channel = std::move(row[3]);
                auto &handlers = getHandlers(toNumber(row[4]), config);
                handlers.emplace_back(std::move(row[5]), toNumber(row[6]), std::move(row[7]));
            }
        }
    }

    DefaultLogger::logInfo("Controller {} bot configurations loaded", configurations.size());
    return configurations;
}

BotConfiguration DBController::loadBotConfiguration(int id) {
    static const std::string request = fmt::format("SELECT bot.id, bot.user_id, accounts.username, "
                                                   "       channel.name, eh.event_type_id, "
                                                   "       eh.script, eh.id, eh.additional "
                                                   "FROM bot "
                                                   "JOIN bot_account ON bot.id = bot_account.bot_id "
                                                   "JOIN accounts ON bot_account.account_id = accounts.id "
                                                   "JOIN bot_channel ON bot.id = bot_channel.bot_id "
                                                   "JOIN channel ON bot_channel.channel_id = channel.id "
                                                   "JOIN event_handler as eh on bot.id = eh.bot_id "
                                                   "WHERE bot.id = {};", id);

    BotConfiguration config;
    {
        DBConnectionLock dbl(pg);
        if (!dbl->ping())
            return config;

        std::vector<std::vector<std::string>> res;
        if (dbl->request(request, res)) {
            if (res.empty())
                return config;

            using namespace Utils::String;
            config.botId = toNumber(res.front()[0]);
            config.userId = toNumber(res.front()[1]);
            config.account = std::move(res.front()[2]);
            config.channel = std::move(res.front()[3]);
            for(auto &row : res) {
                auto &handlers = getHandlers(toNumber(row[4]), config);
                handlers.emplace_back(std::move(row[5]), toNumber(row[6]), std::move(row[7]));
            }
        }
    }

    DefaultLogger::logInfo("Controller bot(id={}) configuration loaded", id);
    return config;
}

DBController::UpdatedChannels DBController::loadChannels(long long &timestamp) {
    std::string request = fmt::format("SELECT name, watch FROM channel WHERE updated > to_timestamp({});", timestamp);

    DBController::UpdatedChannels channels;
    {
        DBConnectionLock dbl(pg);
        if (!dbl->ping())
            return channels;

        std::vector<std::vector<std::string>> res;
        if (dbl->request(request, res)) {
            using Utils::String::toBool;
            std::transform(res.begin(), res.end(), std::inserter(channels, channels.end()),
                [] (auto &row) { return std::make_pair(std::move(row[0]), toBool(row[1])); });
        }
    }

    timestamp = CurrentTime<std::chrono::system_clock>::seconds();
    DefaultLogger::logInfo("Controller {} watched channels loaded", channels.size());
    return channels;
}
