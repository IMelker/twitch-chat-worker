//
// Created by imelker on 01.04.2021.
//

#include "nlohmann/json.hpp"

#include <so_5/send_functions.hpp>

#include "common/Logger.h"
#include "common/Clock.h"
#include "DBController.h"
#include "IRCController.h"

#include <utility>

using json = nlohmann::json;

IRCController::IRCController(const context_t &ctx,
                             so_5::mbox_t processor,
                             so_5::mbox_t http,
                             const IRCConnectConfig &conConfig,
                             std::shared_ptr<DBController> db,
                             std::shared_ptr<Logger> logger)
  : so_5::agent_t(ctx), processor(std::move(processor)), http(std::move(http)),
    logger(std::move(logger)), db(std::move(db)), config(conConfig) {

    auto chs = this->db->loadChannels();
    std::transform(chs.begin(), chs.end(), std::inserter(this->watchChannels, this->watchChannels.end()),
        [] (std::string& channel) {
            return std::make_pair(std::move(channel), nullptr);
    });
    lastChannelLoadTimestamp = CurrentTime<std::chrono::system_clock>::seconds();
}

IRCController::~IRCController() = default;


void IRCController::so_define_agent() {
    so_subscribe_self().event(&IRCController::evtSendMessage);
    so_subscribe_self().event(&IRCController::evtWorkerConnected);
    so_subscribe_self().event(&IRCController::evtWorkerDisconnected);
    so_subscribe_self().event(&IRCController::evtWorkerLogin);
    so_subscribe(http).event(&IRCController::evtHttpStatus);
    so_subscribe(http).event(&IRCController::evtHttpReload);
    so_subscribe(http).event(&IRCController::evtHttpChannelJoin);
    so_subscribe(http).event(&IRCController::evtHttpChannelLeave);
    so_subscribe(http).event(&IRCController::evtHttpChannelMessage);
    so_subscribe(http).event(&IRCController::evtHttpChannelCustom);
    so_subscribe(http).event(&IRCController::evtHttpChannelStats);
    so_subscribe(http).event(&IRCController::evtHttpAccountAdd);
    so_subscribe(http).event(&IRCController::evtHttpAccountRemove);
    so_subscribe(http).event(&IRCController::evtHttpAccountReload);
    so_subscribe(http).event(&IRCController::evtHttpAccountStats);
}

void IRCController::so_evt_start() {
    auto accounts = this->db->loadAccounts();

    so_5::introduce_child_coop(*this, [&] (so_5::coop_t &coop) {
        for (auto &account : accounts) {
            auto *worker = coop.make_agent<IRCWorker>(so_direct_mbox(), processor, config, account, this->logger);
            workers.emplace(account.nick, worker);
        }
    });

    this->logger->logInfo("IRCWorkerPool {} workers inited", workers.size());
}

void IRCController::so_evt_finish() {

}

void IRCController::evtSendMessage(mhood_t<SendMessage> message) {
    auto it = workers.find(message->account);
    if (it != workers.end()) {
        so_5::send(it->second->so_direct_mbox(), message);
    } else {
        logger->logWarn("Failed to find IRC worker for account: ", message->account);
    }
}

void IRCController::evtWorkerConnected(so_5::mhood_t<WorkerConnected> evt) {
    logger->logTrace("IRCWorkerPool::IRCWorker[{}] on connected", fmt::ptr(evt->worker));
}

void IRCController::evtWorkerDisconnected(so_5::mhood_t<WorkerDisconnected> evt) {
    logger->logTrace("IRCWorkerPool::IRCWorker[{}] on disconnected", fmt::ptr(evt->worker));
}

void IRCController::evtWorkerLogin(so_5::mhood_t<WorkerLoggedIn> evt) {
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

void IRCController::evtHttpStatus(mhood_t<hreq::irc::stats> evt) {

}

void IRCController::evtHttpReload(mhood_t<hreq::irc::reload> evt) {

}

void IRCController::evtHttpChannelJoin(mhood_t<hreq::irc::channel::join> evt) {

}

void IRCController::evtHttpChannelLeave(mhood_t<hreq::irc::channel::leave> evt) {

}

void IRCController::evtHttpChannelMessage(mhood_t<hreq::irc::channel::message> evt) {

}

void IRCController::evtHttpChannelCustom(mhood_t<hreq::irc::channel::custom> evt) {

}

void IRCController::evtHttpChannelStats(mhood_t<hreq::irc::channel::stats> evt) {

}

void IRCController::evtHttpAccountAdd(mhood_t<hreq::irc::account::add> evt) {

}

void IRCController::evtHttpAccountRemove(mhood_t<hreq::irc::account::remove> evt) {

}

void IRCController::evtHttpAccountReload(mhood_t<hreq::irc::account::reload> evt) {

}

void IRCController::evtHttpAccountStats(mhood_t<hreq::irc::account::stats> evt) {

}

std::tuple<int, std::string> IRCController::processHttpRequest(std::string_view path, const std::string &request,
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
    return {};
}

std::string IRCController::handleJoin(const std::string &request, std::string &error) {
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

std::string IRCController::handleAccounts(const std::string &request, std::string &error) {
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

std::string IRCController::handleChannels(const std::string &request, std::string &error) {
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

std::string IRCController::handleLeave(const std::string &request, std::string &error) {
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

std::string IRCController::handleReloadChannels(const std::string &request, std::string &error) {
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

std::string IRCController::handleMessage(const std::string &request, std::string &error) {
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

std::string IRCController::handleCustom(const std::string &request, std::string &error) {
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
