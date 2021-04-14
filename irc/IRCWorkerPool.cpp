//
// Created by imelker on 01.04.2021.
//

#include "nlohmann/json.hpp"

#include <so_5/send_functions.hpp>

#include "../common/Logger.h"
#include "../common/Clock.h"
#include "IRCWorkerPool.h"

#include <utility>

using json = nlohmann::json;

IRCWorkerPool::IRCWorkerPool(const context_t &ctx, so_5::mbox_t processor,
                             const IRCConnectConfig &conConfig,
                             DBController *db,
                             std::shared_ptr<Logger> logger)
  : so_5::agent_t(ctx), processor(std::move(processor)), logger(std::move(logger)), config(conConfig), db(db) {

    auto chs = db->loadChannels();
    std::transform(chs.begin(), chs.end(), std::inserter(this->watchChannels, this->watchChannels.end()),
        [] (std::string& channel) {
            return std::make_pair(std::move(channel), nullptr);
    });
    lastChannelLoadTimestamp = CurrentTime<std::chrono::system_clock>::seconds();
}

IRCWorkerPool::~IRCWorkerPool() = default;


void IRCWorkerPool::so_define_agent() {
    so_subscribe_self().event(&IRCWorkerPool::evtSendMessage);
    so_subscribe_self().event(&IRCWorkerPool::evtWorkerConnected);
    so_subscribe_self().event(&IRCWorkerPool::evtWorkerDisconnected);
    so_subscribe_self().event(&IRCWorkerPool::evtWorkerLogin);
}

void IRCWorkerPool::so_evt_start() {
    auto accounts = this->db->loadAccounts();

    so_5::introduce_child_coop(*this, [&] (so_5::coop_t &coop) {
        for (auto &account : accounts) {
            auto *worker = coop.make_agent<IRCWorker>(so_direct_mbox(), processor, config, account, this->logger);
            workers.emplace(account.nick, worker);
        }
    });

    this->logger->logInfo("IRCWorkerPool {} workers inited", workers.size());
}

void IRCWorkerPool::so_evt_finish() {

}

void IRCWorkerPool::evtSendMessage(mhood_t<SendMessage> message) {
    auto it = workers.find(message->account);
    if (it != workers.end()) {
        so_5::send(it->second->so_direct_mbox(), message);
    } else {
        logger->logWarn("Failed to find IRC worker for account: ", message->account);
    }
}

void IRCWorkerPool::evtWorkerConnected(so_5::mhood_t<WorkerConnected> evt) {
    logger->logTrace("IRCWorkerPool::IRCWorker[{}] on connected", fmt::ptr(evt->worker));
}

void IRCWorkerPool::evtWorkerDisconnected(so_5::mhood_t<WorkerDisconnected> evt) {
    logger->logTrace("IRCWorkerPool::IRCWorker[{}] on disconnected", fmt::ptr(evt->worker));
}

void IRCWorkerPool::evtWorkerLogin(so_5::mhood_t<WorkerLoggedIn> evt) {
    logger->logTrace("IRCWorkerPool::IRCWorker[{}] on logged in", fmt::ptr(evt->worker));

    int counter = evt->worker->freeChannelSpace();
    if (counter == 0)
        return;

    for(auto &[channel, attached]: watchChannels) {
        if (attached && attached != evt->worker) continue;

        so_5::send<JoinChannel>(evt->worker->so_direct_mbox(), channel);
        attached = evt->worker;

        if (--counter == 0)
            break;
    }
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
    auto *worker = it->second;
    int channelSpace = worker->freeChannelSpace();

    json body = json::object();
    auto tryToJoinToChannel = [worker, &body, &channelSpace] (const std::string& channel) {
        std::string result;
        bool joined = (--channelSpace == 0);
        if (joined)
            so_5::send<JoinChannel>(worker->so_direct_mbox(), channel);
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

    auto *worker = it->second;

    const auto &channels = req["channels"];
    switch (channels.type()) {
        case json::value_t::array: {
            for (const auto &channel: channels) {
                if (!channel.is_string())
                    continue;
                so_5::send<LeaveChannel>(worker->so_direct_mbox(), channel.get<std::string>());
            }
            break;
        }
        case json::value_t::string: {
            so_5::send<LeaveChannel>(worker->so_direct_mbox(), channels.get<std::string>());
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
        //std::lock_guard lg(watchMutex);
        for (auto &[channel, watch]: channels) {
            if (watch) {
                joinList.push_back(channel);

                auto it = watchChannels.find(channel);
                if (it == watchChannels.end()) {
                    it = watchChannels.insert(std::make_pair(channel, nullptr)).first;
                }

                if (it->second)
                    continue;

                for (auto &[acc, irc]: workers) {
                    int space = irc->freeChannelSpace();
                    if (space == 0)
                        continue;

                    so_5::send<JoinChannel>(irc->so_direct_mbox(), channel);
                    it->second = irc;
                    ++joined;
                    break;
                }
            } else {
                leaveList.push_back(channel);

                auto it = watchChannels.find(channel);
                if (it == watchChannels.end())
                    continue;

                if (it->second) {
                    so_5::send<LeaveChannel>(it->second->so_direct_mbox(), channel);
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
                    so_5::send<SendMessage>(worker->so_direct_mbox(), worker->getClientConfig().nick, chan, text);
                    res[chan] = true;
                }
                break;
            case json::value_t::string: {
                const auto &chan = channels.get<std::string>();
                so_5::send<SendMessage>(worker->so_direct_mbox(), worker->getClientConfig().nick, chan, text);
                res[chan] = true;
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

                body[acc] = sendTextToChannels(it->second, text);
            }
            break;
        case json::value_t::string: {
            const auto& acc = accounts.get<std::string>();
            auto it = workers.find(acc);
            if (it == workers.end()) {
                body[acc] = false;
                break;
            }

            body[acc] = sendTextToChannels(it->second, text);
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

                logger->logInfo(R"(IRCWorker[{}] {} send command: "{}")", fmt::ptr(it->second), acc, command);
                so_5::send<SendIRC>(it->second->so_direct_mbox(), command);
                body[acc] = true;
            }
            break;
        case json::value_t::string: {
            const auto& acc = accounts.get<std::string>();
            auto it = workers.find(acc);
            if (it == workers.end()) {
                body[acc] = false;
                break;
            }

            logger->logInfo(R"(IRCWorker[{}] {} send command: "{}")", fmt::ptr(it->second), acc, command);
            so_5::send<SendIRC>(it->second->so_direct_mbox(), command);
            body[acc] = true;
            break;
        }
        default:
            break;
    }

    return body.dump();
}
