//
// Created by l2pic on 06.03.2021.
//

#include <algorithm>
#include <chrono>
#include <ctime>

#include "nlohmann/json.hpp"

#include "common/SysSignal.h"
#include "common/Logger.h"

#include "db/DBConnectionLock.h"
#include "IRCtoDBConnector.h"

using json = nlohmann::json;

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
        std::string nick = account.nick;
        workers.emplace(std::move(nick), std::make_shared<IRCWorker>(config, std::move(account), this, ircLogger));
    }
}

const std::shared_ptr<PGConnectionPool> & IRCtoDBConnector::getPG() const {
    return pg;
}

const std::shared_ptr<CHConnectionPool> & IRCtoDBConnector::getCH() const {
    return ch;
}

ThreadPool *IRCtoDBConnector::getPool() {
    return &pool;
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

    std::lock_guard lg(watchMutex);
    this->watchChannels = std::move(channels);
}

void IRCtoDBConnector::onConnected(IRCWorker *worker) {
    logger->logTrace("IRCtoDBConnector IRCWorker[{}] connected", fmt::ptr(worker));
}

void IRCtoDBConnector::onDisconnected(IRCWorker *worker) {
    logger->logTrace("IRCtoDBConnector IRCWorker[{}] disconnected", fmt::ptr(worker));
}

void IRCtoDBConnector::onMessage(IRCWorker *worker, IRCMessage message, long long now) {
    pool.enqueue([message = std::move(message), now, worker, this]() {
        MessageData transformed;
        transformed.timestamp = now;
        msgProcessor.transformMessage(message, transformed);

        logger->logTrace(R"(IRCtoDBConnector process {{worker: {}, channel: "{}", from: "{}", text: "{}", valid: {}}})",
                         fmt::ptr(worker), transformed.channel, transformed.user, transformed.text, transformed.valid);

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

    std::lock_guard lg(watchMutex);
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

std::tuple<int, std::string> IRCtoDBConnector::processHttpRequest(std::string_view path, const std::string &request, std::string &error) {
    if (path == "shutdown") {
        SysSignal::setServiceTerminated(true);
        return {200, ""};
    }

    if (path == "join") {
        json req = json::parse(request, nullptr, false, true);
        if (req.is_discarded()) {
            error = "Failed to parse JSON";
            return EMPTY_HTTP_RESPONSE;
        }

        const auto& account = req["account"];
        if (!account.is_string()) {
            error = "Account not found";
            return EMPTY_HTTP_RESPONSE;
        }
        auto it = workers.find(account.get<std::string_view>());
        if (it == workers.end()) {
            error = "Account not found";
            return EMPTY_HTTP_RESPONSE;
        }

        json body = json::object();
        const auto &channels = req["channels"];
        switch (channels.type()) {
            case json::value_t::array:
                for (const auto &channel: channels) {
                    if (!channel.is_string())
                        continue;
                    const auto &chan = channel.get<std::string>();
                    body[chan] = it->second->joinChannel(chan);
                }
                break;
            case json::value_t::string: {
                const auto &chan = channels.get<std::string>();
                body[chan] = it->second->joinChannel(chan);
                break;
            }
            default:
                break;
        }

        return {200, body.dump()};
    }

    if (path == "leave") {
        json req = json::parse(request, nullptr, false, true);
        if (req.is_discarded()) {
            error = "Failed to parse JSON";
            return EMPTY_HTTP_RESPONSE;
        }

        const auto& account = req["account"];
        if (!account.is_string()) {
            error = "Account not found";
            return EMPTY_HTTP_RESPONSE;
        }
        auto it = workers.find(account.get<std::string_view>());
        if (it == workers.end()) {
            error = "Account not found";
            return EMPTY_HTTP_RESPONSE;
        }

        const auto &channels = req["channels"];
        switch (channels.type()) {
            case json::value_t::array:
                for (const auto &channel: channels) {
                    if (!channel.is_string())
                        continue;
                    const auto &chan = channel.get<std::string>();
                    it->second->leaveChannel(chan);
                }
                break;
            case json::value_t::string: {
                const auto &chan = channels.get<std::string>();
                it->second->leaveChannel(chan);
                break;
            }
            default:
                break;
        }

        return {200, ""};
    }

    if (path == "accounts") {
        auto fillAccount = [] (json& account, const auto& stats) {
            account["connects"] = {{"updated", stats.connects.timestamp.load(std::memory_order_relaxed)},
                                   {"count", stats.connects.count.load(std::memory_order_relaxed)}};
            account["channels"] = {{"updated", stats.channels.timestamp.load(std::memory_order_relaxed)},
                                   {"count", stats.channels.count.load(std::memory_order_relaxed)}};
            account["messages"]["in"] = {{"updated", stats.messages.in.timestamp.load(std::memory_order_relaxed)},
                                         {"count", stats.messages.in.count.load(std::memory_order_relaxed)}};
            account["messages"]["out"] = {{"updated", stats.messages.out.timestamp.load(std::memory_order_relaxed)},
                                          {"count", stats.messages.out.count.load(std::memory_order_relaxed)}};
        };

        json body = json::object();
        if (request.empty()) {
            for (auto &[nick, worker]: workers)
                fillAccount(body[nick], worker->getStats());
        } else {
            json req = json::parse(request, nullptr, false, true);
            if (req.is_discarded()) {
                error = "Failed to parse JSON";
                return EMPTY_HTTP_RESPONSE;
            }

            const auto& accounts = req["accounts"];
            switch (accounts.type()) {
                case json::value_t::array:
                    for (const auto &account: accounts) {
                        if (!account.is_string())
                            continue;

                        if (auto it = workers.find(account.get<std::string_view>()); it != workers.end())
                            fillAccount(body[it->first], it->second->getStats());
                    }
                    break;
                case json::value_t::string:
                    if (auto it = workers.find(accounts.get<std::string_view>()); it != workers.end())
                        fillAccount(body[it->first], it->second->getStats());
                    break;
                default:
                    break;
            }
        }

        return {200, body.dump()};
    }

    if (path == "channels") {
        json body = json::object();
        if (request.empty()) {
            for (auto &[nick, worker]: workers)
                body[nick] = json(worker->getJoinedChannels());
        } else {
            json req = json::parse(request, nullptr, false, true);
            if (req.is_discarded()) {
                error = "Failed to parse JSON";
                return EMPTY_HTTP_RESPONSE;
            }

            const auto& accounts = req["accounts"];
            switch (accounts.type()) {
                case json::value_t::array:
                    for (const auto &account: accounts) {
                        if (!account.is_string())
                            continue;

                        if (auto it = workers.find(account.get<std::string_view>()); it != workers.end())
                            body[it->first] = json(it->second->getJoinedChannels());
                    }
                    break;
                case json::value_t::string:
                    if (auto it = workers.find(accounts.get<std::string_view>());it != workers.end())
                        body[it->first] = json(it->second->getJoinedChannels());
                    break;
                default:
                    break;
            }
        }
        return {200, body.dump()};
    }

    error = "Failed to match path";
    return EMPTY_HTTP_RESPONSE;
}
