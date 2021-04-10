//
// Created by imelker on 01.04.2021.
//

#include "nlohmann/json.hpp"

#include "../common/Logger.h"
#include "../common/Clock.h"
#include "IRCWorkerPool.h"

using json = nlohmann::json;

IRCWorkerPool::IRCWorkerPool(const IRCConnectConfig &conConfig, DBController *db,
                             IRCMessageListener *listener, std::shared_ptr<Logger> logger)
  : listener(listener), logger(std::move(logger)), db(db) {

    auto chs = db->loadChannels();
    std::transform(chs.begin(), chs.end(), std::inserter(this->watchChannels, this->watchChannels.end()),
        [] (std::string& channel) {
            return std::make_pair(std::move(channel), nullptr);
    });

    lastChannelLoadTimestamp = CurrentTime<std::chrono::system_clock>::seconds();

    auto accounts = this->db->loadAccounts();
    for (auto &account : accounts) {
        auto worker = std::make_shared<IRCWorker>(conConfig, std::move(account), this, this->logger);
        workers.emplace(worker->getClientConfig().nick, worker);
    }
    this->logger->logInfo("IRCWorkerPool {} workers inited", workers.size());
}

IRCWorkerPool::~IRCWorkerPool() = default;

size_t IRCWorkerPool::poolSize() const {
    return workers.size();
}


void IRCWorkerPool::sendMessage(const std::string &account,
                                const std::string &channel,
                                const std::string &text) {
    auto it = workers.find(account);
    if (it != workers.end()) {
        it->second->sendMessage(channel, text);
    } else {
        logger->logWarn("Failed to find IRC worker for account: ", account);
    }
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
    for(auto &[channel, attached]: watchChannels) {
        if (attached) continue;

        if (worker->channelLimitReached())
            break;

        if (worker->joinChannel(channel, dev_null)) {
            attached = worker;
        } else {
            break;
        }
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
    auto tryToJoinToChannel = [worker, &body] (const std::string& channel) {
        std::string result;
        bool joined = worker->joinChannel(channel, result);
        body[channel] = {{"joined", joined}, {"reason", std::move(result)}};
    };

    const auto &channels = req["channels"];
    switch (channels.type()) {
        case json::value_t::array: {
            for (const auto &channel: channels) {
                if (!channel.is_string())
                    continue;
                tryToJoinToChannel(channel.get<std::string>());
            }
            break;
        }
        case json::value_t::string: {
            tryToJoinToChannel(channels.get<std::string>());
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

    const auto &channels = req["channels"];
    switch (channels.type()) {
        case json::value_t::array: {
            for (const auto &channel: channels) {
                if (!channel.is_string())
                    continue;
                worker->leaveChannel(channel.get<std::string>());
            }
            break;
        }
        case json::value_t::string: {
            worker->leaveChannel(channels.get<std::string>());
            break;
        }
        default:
            break;
    }
    return "";
}

std::string IRCWorkerPool::handleReloadChannels(const std::string &request, std::string &error) {
    auto channels = this->db->loadChannels(lastChannelLoadTimestamp);

    std::vector<std::string> leaveList, joinList;
    int leaved = 0, joined = 0;

    {
        std::lock_guard lg(watchMutex);
        for (auto &[channel, watch]: channels) {
            if (watch) {
                joinList.push_back(channel);

                auto it = watchChannels.find(channel);
                if (it == watchChannels.end()) {
                    it = watchChannels.insert(std::make_pair(channel, nullptr)).first;
                }

                if (it->second)
                    continue;

                std::string dev_null;
                for (auto &[acc, irc]: workers) {
                    if (irc->channelLimitReached())
                        continue;

                    if (irc->joinChannel(channel, dev_null)) {
                        it->second = irc.get();
                        ++joined;
                    }
                }
            } else {
                leaveList.push_back(channel);

                auto it = watchChannels.find(channel);
                if (it == watchChannels.end())
                    continue;

                if (it->second) {
                    it->second->leaveChannel(channel);
                    ++leaved;
                }
                watchChannels.erase(it);
            }
        }
    }

    logger->logInfo("Controller Channels reloaded: left: {}, joined: {}", leaved, joined);
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
