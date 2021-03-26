//
// Created by l2pic on 06.03.2021.
//
#include "app.h"

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

        std::vector<std::vector<std::string>> res;
        if (dbl->request(request, res)) {
            accounts.reserve(res.size());
            for (auto &row: res) {
                IRCClientConfig config;
                config.nick = std::move(row[0]);
                config.user = std::move(row[1]);
                config.password = std::move(row[2]);
                config.channels_limit = std::atoi(row.at(3).c_str());
                config.whisper_per_sec_limit = std::atoi(row.at(4).c_str());
                config.auth_per_sec_limit = std::atoi(row.at(5).c_str());
                config.command_per_sec_limit = std::atoi(row.at(6).c_str());

                accounts.push_back(std::move(config));
            }
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

        std::vector<std::vector<std::string>> res;
        if (dbl->request(request, res)) {
            channelsList.reserve(res.size());
            for(auto& row: res) {
                channelsList.push_back(std::move(row[0]));
            }
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
    if (path == "message")
        return {200, handleMessage(request, error)};
    if (path == "custom")
        return {200, handleCustom(request, error)};
    if (path == "join")
        return {200, handleJoin(request, error)};
    if (path == "accounts")
        return {200, handleAccounts(request, error)};
    if (path == "channels")
        return {200, handleChannels(request, error)};
    if (path == "leave")
        return {200, handleLeave(request, error)};
    if (path == "version")
        return {200, handleVersion(request, error)};
    if (path == "shutdown")
        return {200, handleShutdown(request, error)};

    error = "Failed to match path";
    return EMPTY_HTTP_RESPONSE;
}


std::string IRCtoDBConnector::handleShutdown(const std::string &, std::string &) {
    SysSignal::setServiceTerminated(true);
    return "Terminated";
}

std::string IRCtoDBConnector::handleJoin(const std::string &request, std::string &error) {
    json req = json::parse(request, nullptr, false, true);
    if (req.is_discarded()) {
        error = "Failed to parse JSON";
        return "";
    }

    const auto& account = req["account"];
    if (!account.is_string()) {
        error = "Account not found";
        return "";
    }
    auto it = workers.find(account.get<std::string_view>());
    if (it == workers.end()) {
        error = "Account not found";
        return "";
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
    return body.dump();
}

std::string IRCtoDBConnector::handleAccounts(const std::string &request, std::string &error) {
    auto fillAccount = [] (json& account, const auto& stats) {
        account["connects"] = {{"updated", stats.connects.updated.load(std::memory_order_relaxed)},
                               {"count", stats.connects.attempts.load(std::memory_order_relaxed)}};
        account["channels"] = {{"updated", stats.channels.updated.load(std::memory_order_relaxed)},
                               {"count", stats.channels.count.load(std::memory_order_relaxed)}};
        account["messages"]["in"] = {{"updated", stats.messages.in.updated.load(std::memory_order_relaxed)},
                                     {"count", stats.messages.in.count.load(std::memory_order_relaxed)}};
        account["messages"]["out"] = {{"updated", stats.messages.out.updated.load(std::memory_order_relaxed)},
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
            return "";
        }

        const auto& accounts = req["accounts"];
        switch (accounts.type()) {
            case json::value_t::array:
                for (const auto &account: accounts) {
                    if (!account.is_string())
                        continue;

                    auto it = workers.find(account.get<std::string_view>());
                    if (it != workers.end())
                        fillAccount(body[it->first], it->second->getStats());
                }
                break;
            case json::value_t::string: {
                auto it = workers.find(accounts.get<std::string_view>());
                if (it != workers.end())
                    fillAccount(body[it->first], it->second->getStats());
                break;
            }
            default:
                break;
        }
    }
    return body.dump();
}

std::string IRCtoDBConnector::handleChannels(const std::string &request, std::string &error) {
    json body = json::object();
    if (request.empty()) {
        for (auto &[nick, worker]: workers)
            body[nick] = json(worker->getJoinedChannels());
    } else {
        json req = json::parse(request, nullptr, false, true);
        if (req.is_discarded()) {
            error = "Failed to parse JSON";
            return "";
        }

        const auto& accounts = req["accounts"];
        switch (accounts.type()) {
            case json::value_t::array:
                for (const auto &account: accounts) {
                    if (!account.is_string())
                        continue;

                    auto it = workers.find(account.get<std::string_view>());
                    if (it != workers.end())
                        body[it->first] = json(it->second->getJoinedChannels());
                }
                break;
            case json::value_t::string:{
                auto it = workers.find(accounts.get<std::string_view>());
                if (it != workers.end())
                    body[it->first] = json(it->second->getJoinedChannels());
                break;
            }
            default:
                break;
        }
    }
    return body.dump();
}

std::string IRCtoDBConnector::handleLeave(const std::string &request, std::string &error) {
    json req = json::parse(request, nullptr, false, true);
    if (req.is_discarded()) {
        error = "Failed to parse JSON";
        return "";
    }

    const auto& account = req["account"];
    if (!account.is_string()) {
        error = "Account not found";
        return "";
    }
    auto it = workers.find(account.get<std::string_view>());
    if (it == workers.end()) {
        error = "Account not found";
        return "";
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
    return "";
}

std::string IRCtoDBConnector::handleMessage(const std::string &request, std::string &error) {
    json req = json::parse(request, nullptr, false, true);
    if (req.is_discarded()) {
        error = "Failed to parse JSON";
        return "";
    }

    const auto& text = req["text"];
    if (!text.is_string()) {
        error = "Invalid text type";
        return "";
    }

    json body = json::object();
    auto sendTextToChannels = [&req] (IRCWorker *worker, const std::string& text) {
        json res = json::object();
        const auto &channels = req["channels"];
        switch (channels.type()) {
            case json::value_t::array:
                for (const auto &channel: channels) {
                    if (!channel.is_string())
                        continue;
                    const auto &chan = channel.get<std::string>();
                    res[chan] = worker->sendMessage(chan, text);
                }
                break;
            case json::value_t::string: {
                const auto &chan = channels.get<std::string>();
                res[chan] = worker->sendMessage(chan, text);
                break;
            }
            default:
                break;
        }
        return res;
    };


    const auto &accounts = req["accounts"];
    switch (accounts.type()) {
        case json::value_t::array:
            for (const auto &account: accounts) {
                if (!account.is_string())
                    continue;

                const auto& acc = account.get<std::string>();
                auto it = workers.find(acc);
                if (it == workers.end()) {
                    body[acc] = false;
                    continue;
                }

                body[acc] = sendTextToChannels(it->second.get(), text);
            }
            break;
        case json::value_t::string: {
            const auto& acc = accounts.get<std::string>();
            auto it = workers.find(acc);
            if (it == workers.end()) {
                body[acc] = false;
                break;
            }

            body[acc] = sendTextToChannels(it->second.get(), text);
            break;
        }
        default:
            break;
    }

    return body.dump();
}

std::string IRCtoDBConnector::handleCustom(const std::string &request, std::string &error) {
    json req = json::parse(request, nullptr, false, true);
    if (req.is_discarded()) {
        error = "Failed to parse JSON";
        return "";
    }

    const auto& command = req["command"];
    if (!command.is_string()) {
        error = "Invalid command type";
        return "";
    }

    json body = json::object();
    const auto &accounts = req["accounts"];
    switch (accounts.type()) {
        case json::value_t::array:
            for (const auto &account: accounts) {
                if (!account.is_string())
                    continue;

                const auto& acc = account.get<std::string>();
                auto it = workers.find(acc);
                if (it == workers.end()) {
                    body[acc] = false;
                    continue;
                }

                logger->logInfo(R"(IRCWorker[{}] "{} send command: {})", fmt::ptr(it->second.get()), acc, command);
                body[acc] = it->second->sendIRC(command);
            }
            break;
        case json::value_t::string: {
            const auto& acc = accounts.get<std::string>();
            auto it = workers.find(acc);
            if (it == workers.end()) {
                body[acc] = false;
                break;
            }

            logger->logInfo(R"(IRCWorker[{}] "{} send command: "{}")", fmt::ptr(it->second.get()), acc, command);
            body[acc] = it->second->sendIRC(command);
            break;
        }
        default:
            break;
    }

    return body.dump();
}

std::string IRCtoDBConnector::handleVersion(const std::string &, std::string &) {
    json body = json::object();
    body["version"] = APP_NAME " " APP_VERSION " " APP_GIT_DATE " (" APP_GIT_HASH ")";
    return body.dump();
}
