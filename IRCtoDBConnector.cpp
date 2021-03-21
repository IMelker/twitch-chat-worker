//
// Created by l2pic on 06.03.2021.
//

#include <algorithm>
#include <chrono>
#include <ctime>

#include "common/SysSignal.h"
#include "common/Logger.h"

#include "db/DBConnectionLock.h"
#include "IRCtoDBConnector.h"

std::vector<IRCClientConfig> getAccountsList(const std::shared_ptr<PGConnectionPool>& pgBackend) {
    auto &logger = pgBackend->getLogger();

    std::string request = "SELECT username, display, oauth, channels_limit, whisper_per_sec_limit,"
                                 "auth_per_sec_limit, command_per_sec_limit "
                           "FROM accounts "
                           "WHERE active IS TRUE;";
    logger->logTrace("PGConnection request: \"{}\"", request);

    std::vector<IRCClientConfig> accounts;
    {
        DBConnectionLock dbl(pgBackend);
        if (!dbl->ping())
            return accounts;

        PQsendQuery(dbl->raw(), request.c_str());
        while (auto resp = PQgetResult(dbl->raw())) {
            auto status = PQresultStatus(resp);
            if (status == PGRES_TUPLES_OK) {
                int size = PQntuples(resp);
                logger->logTrace("PGConnection response: {} lines {}", PQresStatus(status), size);

                accounts.reserve(size);
                for (int i = 0; i < size; ++i) {
                    IRCClientConfig config;
                    config.nick = PQgetvalue(resp, i, 0);
                    config.user = PQgetvalue(resp, i, 1);
                    config.password = PQgetvalue(resp, i, 2);
                    config.channels_limit = std::atoi(PQgetvalue(resp, i, 3));
                    config.whisper_per_sec_limit = std::atoi(PQgetvalue(resp, i, 4));
                    config.auth_per_sec_limit = std::atoi(PQgetvalue(resp, i, 5));
                    config.command_per_sec_limit = std::atoi(PQgetvalue(resp, i, 6));

                    accounts.push_back(std::move(config));
                }
            }

            if (status == PGRES_FATAL_ERROR) {
                logger->logError("DBConnection Error: {}\n", PQresultErrorMessage(resp));
            }
            PQclear(resp);
        }
    }

    return accounts;
}

std::vector<std::string> getChannelsList(const std::shared_ptr<PGConnectionPool>& pgBackend) {
    auto &logger = pgBackend->getLogger();

    std::string request = "SELECT name FROM channel WHERE watch IS TRUE;";
    logger->logTrace("PGConnection request: \"{}\"", request);

    std::vector<std::string> channelsList;
    {
        DBConnectionLock dbl(pgBackend);
        if (!dbl->ping())
            return channelsList;

        PQsendQuery(dbl->raw(), request.c_str());
        while (auto resp = PQgetResult(dbl->raw())) {
            auto status = PQresultStatus(resp);
            if (status == PGRES_TUPLES_OK) {
                int size = PQntuples(resp);
                logger->logTrace("PGConnection response: {} lines {}", PQresStatus(status), size);

                channelsList.reserve(size);
                for (int i = 0; i < size; ++i) {
                    channelsList.emplace_back(PQgetvalue(resp, i, 0));
                }
            }

            if (PQresultStatus(resp) == PGRES_FATAL_ERROR) {
                logger->logError("PGConnection Error: {}\n", PQresultErrorMessage(resp));
            }
            PQclear(resp);
        }
    }

    return channelsList;
}

IRCtoDBConnector::IRCtoDBConnector(unsigned int threads, std::shared_ptr<Logger> logger)
    : logger(std::move(logger)), pool(threads) {
    this->logger->logInfo("IRCtoDBConnector created with {} threads", threads);
}

IRCtoDBConnector::~IRCtoDBConnector() {
    if (ch)
        dbProcessor.flushMessages(ch);
    logger->logTrace("IRCtoDBConnector end of connector");
};

void IRCtoDBConnector::initPGConnectionPool(PGConnectionConfig config, unsigned int connections, std::shared_ptr<Logger> logger) {
    assert(!this->pg);
    this->logger->logInfo("IRCtoDBConnector init PostgreSQL DB pool with {} connections", connections);
    pg = std::make_shared<PGConnectionPool>(std::move(config), connections, std::move(logger));
}

void IRCtoDBConnector::initCHConnectionPool(CHConnectionConfig config, unsigned int connections, std::shared_ptr<Logger> logger) {
    assert(!this->ch);
    this->logger->logInfo("IRCtoDBConnector init Clickhouse DB pool with {} connections", connections);
    ch = std::make_shared<CHConnectionPool>(std::move(config), connections, std::move(logger));
}

void IRCtoDBConnector::initIRCWorkers(const IRCConnectConfig& config, std::vector<IRCClientConfig> accounts, std::shared_ptr<Logger> ircLogger) {
    assert(workers.empty());

    this->logger->logInfo("IRCtoDBConnector {} users loaded", accounts.size());

    for (auto &account : accounts) {
        workers.push_back(std::make_shared<IRCWorker>(config, std::move(account), this, ircLogger));
    }
}

const std::shared_ptr<PGConnectionPool> & IRCtoDBConnector::getPG() const {
    return pg;
}

const std::shared_ptr<CHConnectionPool> & IRCtoDBConnector::getCH() const {
    return ch;
}


std::vector<IRCClientConfig> IRCtoDBConnector::loadAccounts() {
    std::vector<IRCClientConfig> accounts;

    if (pg)
        accounts = getAccountsList(pg);

    this->logger->logInfo("IRCtoDBConnector {} accounts loaded", accounts.size());
    return accounts;
}


std::vector<std::string> IRCtoDBConnector::loadChannels() {
    std::vector<std::string> channels;

    if (pg)
        channels = getChannelsList(pg);

    this->logger->logInfo("IRCtoDBConnector {} channels loaded", channels.size());
    return channels;
}

void IRCtoDBConnector::updateChannelsList(std::vector<std::string> &&channels) {
    this->logger->logInfo("IRCtoDBConnector watch list updated with {} channels", channels.size());

    std::lock_guard lg(channelsMutex);
    this->watchChannels = std::move(channels);
}

void IRCtoDBConnector::onConnected(IRCWorker *worker) {
    logger->logTrace("IRCtoDBConnector IRCWorker[{}] connected", fmt::ptr(worker));
}

void IRCtoDBConnector::onDisconnected(IRCWorker *worker) {
    logger->logTrace("IRCtoDBConnector IRCWorker[{}] disconnected", fmt::ptr(worker));
}

void IRCtoDBConnector::onMessage(IRCMessage message, IRCWorker *) {
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    pool.enqueue([message = std::move(message), timestamp, this]() {
        MessageData transformed;
        transformed.timestamp = timestamp;
        msgProcessor.transformMessage(message, transformed);

        logger->logTrace(R"(IRCtoDBConnector process {{channel: "{}",from: "{}", text: "{}", valid: {}}})",
                               transformed.channel, transformed.user, transformed.text, transformed.valid);

        // process request to database
        if (transformed.valid) {
            pool.enqueue([msg = std::move(transformed), this]() mutable {
                if (ch) {
                    dbProcessor.processMessage(std::move(msg), ch);
                }
            });
        }
    });
}

void IRCtoDBConnector::onLogin(IRCWorker *worker) {
    logger->logTrace("IRCtoDBConnector IRCWorker[{}] logged in", fmt::ptr(worker));

    std::lock_guard lg(channelsMutex);
    size_t pos = 0;
    for (pos = 0; pos < watchChannels.size(); ++pos) {
        if (!worker->joinChannel(watchChannels[pos]))
            break;
    }
    watchChannels.erase(watchChannels.begin(), watchChannels.begin() + pos);
}

void IRCtoDBConnector::loop() {
    while (!SysSignal::serviceTerminated()){
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

std::tuple<int, std::string> IRCtoDBConnector::processHttpRequest(std::string_view path, const std::string &body, std::string &error) {
    return {200, body};
}
