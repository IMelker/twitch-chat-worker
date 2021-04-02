//
// Created by l2pic on 01.04.2021.
//

#include "nlohmann/json.hpp"

#include "../common/Logger.h"
#include "IRCWorkerPool.h"

using json = nlohmann::json;

IRCWorkerPool::IRCWorkerPool(const IRCConnectConfig &conConfig, DBStorage *db,
                             IRCMessageListener *listener, std::shared_ptr<Logger> logger)
  : listener(listener), logger(std::move(logger)), db(db) {
    auto channels = this->db->loadChannels();
    std::transform(channels.begin(), channels.end(), std::inserter(watchChannels, watchChannels.end()),
        [](std::string& channel) -> std::pair<std::string, IRCWorker *> {
           return std::make_pair(std::move(channel), nullptr);
        });
    this->logger->logInfo("IRCWorkerPool watch list updated with {} channels", watchChannels.size());

    auto accounts = this->db->loadAccounts();
    for (auto &account : accounts) {
        auto worker = std::make_shared<IRCWorker>(conConfig, std::move(account), this, this->logger);
        workers.emplace(worker->getClientConfig().nick, worker);
    }
}

IRCWorkerPool::~IRCWorkerPool() = default;

size_t IRCWorkerPool::poolSize() const {
    return workers.size();
}

void IRCWorkerPool::onConnected(IRCWorker *worker) {
    logger->logTrace("IRCWorkerPool::IRCWorker[{}] connected", fmt::ptr(worker));
}

void IRCWorkerPool::onDisconnected(IRCWorker *worker) {
    logger->logTrace("IRCWorkerPool::IRCWorker[{}] disconnected", fmt::ptr(worker));
}

void IRCWorkerPool::onLogin(IRCWorker *worker) {
    logger->logTrace("IRCWorkerPool::IRCWorker[{}] logged in", fmt::ptr(worker));

    std::string dev_null;

    std::lock_guard lg(watchMutex);
    for(auto &[channel, assigned] : watchChannels) {
        if (assigned)
            continue;

        if (!worker->joinChannel(channel, dev_null))
            break;

        assigned = worker;
    }
}

void IRCWorkerPool::onMessage(IRCWorker *worker, const IRCMessage &message, long long now) {
    listener->onMessage(worker, message, now);
}

std::tuple<int, std::string> IRCWorkerPool::processHttpRequest(std::string_view path, const std::string &request,
                                                               std::string &error) {
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
    if (path == "reloadchannels")
        return {200, handleReloadChannels(request, error)};


    error = "Failed to match path";
    return EMPTY_HTTP_RESPONSE;
}


std::string IRCWorkerPool::handleJoin(const std::string &request, std::string &error) {
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
    auto *worker = it->second.get();

    json body = json::object();
    auto tryToJoinToChannel = [worker, &body, this] (const std::string& channel) {
        auto it = watchChannels.find(channel);
        // check if channel already in list
        if (it != watchChannels.end()) {
            // check if assigned to worker or not
            if (it->second) {
                body[channel] = {{"joined", it->second == worker},
                                 {"reason", it->second->getClientConfig().nick + " already joined"}};
            } else {
                std::string result;
                bool joined = worker->joinChannel(channel, result);
                if (joined)
                    it->second = worker;
                body[channel] = {{"joined", joined}, {"reason", std::move(result)}};
            }
        } else {
            std::string result;
            bool joined = worker->joinChannel(channel, result);
            if (joined)
                watchChannels.insert(std::make_pair(channel, worker));
            body[channel] = {{"joined", joined}, {"reason", std::move(result)}};
        }
    };

    const auto &channels = req["channels"];
    switch (channels.type()) {
        case json::value_t::array: {
            std::lock_guard lg(watchMutex);
            for (const auto &channel: channels) {
                if (!channel.is_string())
                    continue;
                const auto &chan = channel.get<std::string>();
                tryToJoinToChannel(chan);
            }
            break;
        }
        case json::value_t::string: {
            const auto &chan = channels.get<std::string>();
            std::lock_guard lg(watchMutex);
            tryToJoinToChannel(chan);
            break;
        }
        default:
            break;
    }
    return body.dump();
}

std::string IRCWorkerPool::handleAccounts(const std::string &request, std::string &error) {
    auto fillAccount = [] (json& account, const auto& stats) {
        account["reconnects"] = {{"updated", stats.connects.updated.load(std::memory_order_relaxed)},
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

std::string IRCWorkerPool::handleChannels(const std::string &request, std::string &error) {
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

std::string IRCWorkerPool::handleLeave(const std::string &request, std::string &error) {
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

    auto *worker = it->second.get();

    auto tryToJoinToLeave = [worker, this] (const std::string& channel) {
        auto it = watchChannels.find(channel);
        if (it != watchChannels.end()) {
            if (it->second == worker) {
                worker->leaveChannel(channel);
                it->second = nullptr;
            }
        }
    };

    const auto &channels = req["channels"];
    switch (channels.type()) {
        case json::value_t::array: {
            std::lock_guard lg(watchMutex);
            for (const auto &channel: channels) {
                if (!channel.is_string())
                    continue;
                const auto &chan = channel.get<std::string>();
                tryToJoinToLeave(chan);
            }
            break;
        }
        case json::value_t::string: {
            const auto &chan = channels.get<std::string>();
            std::lock_guard lg(watchMutex);
            tryToJoinToLeave(chan);
            break;
        }
        default:
            break;
    }
    return "";
}

std::string IRCWorkerPool::handleReloadChannels(const std::string &, std::string &error) {
    auto channels = this->db->loadChannels();
    if (channels.empty()) {
        error = "Failed to load from db";
        return "";
    }

    int joined = 0;
    std::vector<std::string> leaveList;
    std::vector<std::string> joinList;
    {
        std::lock_guard lg(watchMutex);
        // create leave list
        for (auto& [channel, _]: watchChannels) {
            if (std::find(channels.begin(), channels.end(), channel) == channels.end())
                leaveList.push_back(channel);
        }

        // create join list
        for (auto& channel: channels) {
            if (watchChannels.count(channel) == 0)
                joinList.push_back(channel);
        }

        // leave from channels
        for (auto &channel: leaveList) {
            auto it = watchChannels.find(channel);
            it->second->leaveChannel(channel);
            watchChannels.erase(it);
        }

        // join to channels list
        std::transform(joinList.begin(), joinList.end(), std::inserter(watchChannels, watchChannels.end()),
                       [](std::string &channel) -> std::pair<std::string, IRCWorker *> {
                           return std::make_pair(channel, nullptr);
                       });

        // join workers to channels
        std::string dev_null;
        auto notAssigned = [](const auto& pair) -> bool { return !pair.second; };
        auto it = std::find_if(watchChannels.begin(), watchChannels.end(), notAssigned);
        for (auto&[nick, worker] : workers) {
            while (it != watchChannels.end()) {
                if (!worker->joinChannel(it->first, dev_null))
                    break;

                it->second = worker.get();
                ++joined;

                it = std::find_if(it, watchChannels.end(), notAssigned);
            }
        }
    }

    logger->logInfo("Controller Channels reloaded: left: {}, joined: {}/{}",
                    leaveList.size(), joined, joinList.size());

    json body = {{"leaving", json{leaveList}}, {"left", leaveList.size()}, {"joining", json{joinList}}, {"joined", joined}};
    return body.dump();
}

std::string IRCWorkerPool::handleMessage(const std::string &request, std::string &error) {
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

std::string IRCWorkerPool::handleCustom(const std::string &request, std::string &error) {
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

                logger->logInfo(R"(IRCWorker[{}] {} send command: "{}")", fmt::ptr(it->second.get()), acc, command);
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

            logger->logInfo(R"(IRCWorker[{}] {} send command: "{}")", fmt::ptr(it->second.get()), acc, command);
            body[acc] = it->second->sendIRC(command);
            break;
        }
        default:
            break;
    }

    return body.dump();
}
