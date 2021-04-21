//
// Created by imelker on 01.04.2021.
//

#include "nlohmann/json.hpp"

#include <so_5/send_functions.hpp>

#include "common/Logger.h"
#include "common/Clock.h"
#include "DBController.h"

#include "ChannelController.h"
#include "IRCController.h"

#define resp(str) json{{"result", str}}.dump()

using json = nlohmann::json;

IRCController::IRCController(const context_t &ctx,
                             so_5::mbox_t processor,
                             so_5::mbox_t http,
                             const IRCConnectConfig &conConfig,
                             std::shared_ptr<DBController> db,
                             std::shared_ptr<Logger> logger)
  : so_5::agent_t(ctx), processor(std::move(processor)), http(std::move(http)),
    logger(std::move(logger)), db(std::move(db)), config(conConfig) {

}

IRCController::~IRCController() = default;

void IRCController::so_define_agent() {
    // IRCWorker callback
    so_subscribe_self().event(&IRCController::evtWorkerConnected);
    so_subscribe_self().event(&IRCController::evtWorkerDisconnected);
    so_subscribe_self().event(&IRCController::evtWorkerLogin);

    // from BotEngine
    so_subscribe_self().event(&IRCController::evtSendMessage);

    // from http controller
    so_subscribe(http).event(&IRCController::evtHttpStats);
    so_subscribe(http).event(&IRCController::evtHttpReload);
    so_subscribe(http).event(&IRCController::evtHttpCustom);
    so_subscribe(http).event(&IRCController::evtHttpChannelJoin);
    so_subscribe(http).event(&IRCController::evtHttpChannelLeave);
    so_subscribe(http).event(&IRCController::evtHttpChannelMessage);
    so_subscribe(http).event(&IRCController::evtHttpChannelStats);
    so_subscribe(http).event(&IRCController::evtHttpAccountAdd);
    so_subscribe(http).event(&IRCController::evtHttpAccountRemove);
    so_subscribe(http).event(&IRCController::evtHttpAccountReload);
    so_subscribe(http).event(&IRCController::evtHttpAccountStats);
}

void IRCController::so_evt_start() {
    so_5::introduce_child_coop(*this, [&] (so_5::coop_t &coop) {
        channelController = coop.make_agent<ChannelController>(workersByName, db, logger);
    });

    auto accounts = this->db->loadAccounts();
    for (auto &account : accounts)
        addIrcWorker(account);

    this->logger->logInfo("IRCWorkerPool {} workers inited", workersByName.size());
}

void IRCController::so_evt_finish() {

}

void IRCController::addIrcWorker(const IRCClientConfig& account) {
    auto *worker = so_5::introduce_child_coop(*this, [&account, this] (so_5::coop_t &coop) {
        return coop.make_agent<IRCWorker>(so_direct_mbox(), channelController->so_direct_mbox(), processor, config, account, logger);
    });
    workersByName.emplace(account.nick, worker);
    workersById.emplace(account.id, worker);
}

void IRCController::evtWorkerConnected(so_5::mhood_t<Irc::WorkerConnected> evt) {
    logger->logTrace("IRCWorkerPool::IRCWorker[{}] on connected", fmt::ptr(evt->worker));
}

void IRCController::evtWorkerDisconnected(so_5::mhood_t<Irc::WorkerDisconnected> evt) {
    logger->logTrace("IRCWorkerPool::IRCWorker[{}] on disconnected", fmt::ptr(evt->worker));

    so_5::send(channelController->so_direct_mbox(), evt);
}

void IRCController::evtWorkerLogin(so_5::mhood_t<Irc::WorkerLoggedIn> evt) {
    logger->logTrace("IRCWorkerPool::IRCWorker[{}] on logged in", fmt::ptr(evt->worker));

    so_5::send<Irc::InitAutoJoin>(evt->worker->so_direct_mbox());
}

void IRCController::evtSendMessage(mhood_t<Irc::SendMessage> message) {
    auto it = workersByName.find(message->account);
    if (it != workersByName.end()) {
        so_5::send(it->second->so_direct_mbox(), message);
    } else {
        logger->logWarn("Failed to find IRC worker for account: ", message->account);
    }
}

void IRCController::evtHttpStats(mhood_t<hreq::irc::stats> evt) {
    json body = json::object();
    send_http_resp(http, evt, 200, body.dump());
}

void IRCController::evtHttpReload(mhood_t<hreq::irc::reload> evt) {
    json body = json::object();
    send_http_resp(http, evt, 200, body.dump());
}

void IRCController::evtHttpCustom(mhood_t<hreq::irc::custom> evt) {
    json req = json::parse(evt->req.body(), nullptr, false, true);
    if (req.is_discarded())
        return send_http_resp(http, evt, 400, resp("Failed to parse JSON"));

    const auto& data = req["data"];
    if (!data.is_string())
        return send_http_resp(http, evt, 400, resp("Invalid data type"));

    const auto &accounts = req["accounts"];
    if (!accounts.is_array())
        return send_http_resp(http, evt, 400, resp("Invalid accounts type"));

    json body = json::object();
    for (const auto &account: accounts) {
        if (account.is_string()) {
            const auto& acc = account.get<std::string>();
            auto it = workersByName.find(acc);
            if (it == workersByName.end()) {
                body[acc] = false;
                continue;
            }

            logger->logInfo(R"(IRCWorker[{}] {} send command: "{}")", fmt::ptr(it->second), acc, data);
            so_5::send<Irc::SendIRC>(it->second->so_direct_mbox(), data);
            body[acc] = true;
        } else if (account.is_number()) {
            int id = account.get<int>();
            auto it = workersById.find(id);
            if (it == workersById.end()) {
                body[id] = false;
                continue;
            }

            logger->logInfo(R"(IRCWorker[{}] {} send command: "{}")", fmt::ptr(it->second), id, data);
            so_5::send<Irc::SendIRC>(it->second->so_direct_mbox(), data);
            body[id] = true;
        }
    }

    send_http_resp(http, evt, 200, body.dump());
}

void IRCController::evtHttpChannelJoin(mhood_t<hreq::irc::channel::join> evt) {
    json req = json::parse(evt->req.body(), nullptr, false, true);
    if (req.is_discarded())
        return send_http_resp(http, evt, 400, resp("Failed to parse JSON"));

    IRCWorker *worker = nullptr;
    const auto& account = req["account"];
    if (account.is_string()) {
        auto it = workersByName.find(account.get<std::string_view>());
        if (it == workersByName.end())
            return send_http_resp(http, evt, 404, resp("Account not found"));
        worker = it->second;
    } else if (account.is_number()) {
        auto it = workersById.find(account.get<int>());
        if (it == workersById.end())
            return send_http_resp(http, evt, 404, resp("Account not found"));
        worker = it->second;
    } else {
        return send_http_resp(http, evt, 400, resp("Invalid accounts type"));
    }

    json body = json::object();
    if (worker) {
        const auto &channels = req["channels"];
        if (!channels.is_array())
            return send_http_resp(http, evt, 400, resp("Invalid channels type"));

        for (const auto &channel: channels) {
            if (!channel.is_string())
                continue;

            auto chan = channel.get<std::string>();
            so_5::send<Irc::JoinChannelOnAccount>(channelController->so_direct_mbox(), chan, worker);
            body[chan] = {{"joined", true}};
        }
    }

    send_http_resp(http, evt, 200, body.dump());
}

void IRCController::evtHttpChannelLeave(mhood_t<hreq::irc::channel::leave> evt) {
    json req = json::parse(evt->req.body(), nullptr, false, true);
    if (req.is_discarded())
        return send_http_resp(http, evt, 400, resp("Failed to parse JSON"));

    json body = json::object();
    const auto &channels = req["channels"];
    if (!channels.is_array())
        return send_http_resp(http, evt, 400, resp("Invalid channels type"));

    for (const auto &channel: channels) {
        if (!channel.is_string())
            continue;
        auto chan = channel.get<std::string>();
        so_5::send<Irc::LeaveChannel>(channelController->so_direct_mbox(), chan);
        body[chan] = {{"leaved", true}};
    }

    send_http_resp(http, evt, 200, body.dump());
}

void IRCController::evtHttpChannelMessage(mhood_t<hreq::irc::channel::message> evt) {
    json req = json::parse(evt->req.body(), nullptr, false, true);
    if (req.is_discarded())
        return send_http_resp(http, evt, 400, resp("Failed to parse JSON"));

    const auto& text = req["text"];
    if (!text.is_string())
        return send_http_resp(http, evt, 400, resp("Invalid text type"));

    const auto &accounts = req["accounts"];
    if (!accounts.is_array())
        return send_http_resp(http, evt, 400, resp("Invalid accounts type"));

    const auto &channels = req["channels"];
    if (!channels.is_array())
        return send_http_resp(http, evt, 400, resp("Invalid channels type"));

    json body = json::object();
    for (const auto &account: accounts) {
        IRCWorker *worker = nullptr;
        if (account.is_string()) {
            const auto& acc = account.get<std::string>();
            auto it = workersByName.find(acc);
            if (it == workersByName.end())
                continue;
            worker = it->second;
        } else if (account.is_number()) {
            int id = account.get<int>();
            auto it = workersById.find(id);
            if (it == workersById.end())
                continue;
            worker = it->second;
        }

        if (worker) {
            auto nick = worker->getClientConfig().nick;
            auto msg = text.get<std::string>();
            auto accountBody = body[nick];
            for (const auto &channel: channels) {
                auto chan = channel.get<std::string>();
                logger->logInfo(R"(IRCWorker[{}] {} send message: "{}")", fmt::ptr(worker), chan, msg);
                so_5::send<Irc::SendMessage>(worker->so_direct_mbox(), nick, chan, msg);
                accountBody[chan] = true;
            }
        }
    }

    send_http_resp(http, evt, 200, body.dump());
}

void IRCController::evtHttpChannelStats(mhood_t<hreq::irc::channel::stats> evt) {

}

void IRCController::evtHttpAccountAdd(mhood_t<hreq::irc::account::add> evt) {
    logger->logTrace("IRCController Add new account");

    json req = json::parse(evt->req.body(), nullptr, false, true);
    if (req.is_discarded())
        return send_http_resp(http, evt, 501, resp("Failed to parse JSON"));

    const auto& accId = req["id"];
    if (!accId.is_number())
        return send_http_resp(http, evt, 400, resp("Wrong account id"));

    int id = accId.get<int>();
    auto it = workersById.find(id);
    if (it != workersById.end())
        return send_http_resp(http, evt, 403, resp("Account already added"));

    IRCClientConfig account;
    try {
        account = db->loadAccount(id);
    }
    catch (const std::exception& e) {
        logger->logCritical("DBController Load account exception: {}", e.what());
    }
    if (account.id == 0)
        return send_http_resp(http, evt, 404, resp("Account not found in db"));

    addIrcWorker(account);

    json body = {{"id", id}, {"result", "Account added"}};
    send_http_resp(http, evt, 200, body.dump());
}

void IRCController::evtHttpAccountRemove(mhood_t<hreq::irc::account::remove> evt) {
    json req = json::parse(evt->req.body(), nullptr, false, true);
    if (req.is_discarded())
        return send_http_resp(http, evt, 501, resp("Failed to parse JSON"));

    const auto& accId = req["id"];
    if (!accId.is_number())
        return send_http_resp(http, evt, 400, resp("Wrong account id"));

    int id = accId.get<int>();
    auto it = workersById.find(id);
    if (it == workersById.end())
        return send_http_resp(http, evt, 404, resp("Account not found"));

    workersByName.erase(it->second->getClientConfig().nick);
    so_5::send<Irc::Shutdown>(it->second->so_direct_mbox());
    workersById.erase(it);

    logger->logTrace("IRCController Remove account(id={})", id);

    json body = {{"id", id}, {"result", "Account removed"}};
    send_http_resp(http, evt, 200, body.dump());
}

void IRCController::evtHttpAccountReload(mhood_t<hreq::irc::account::reload> evt) {
    json req = json::parse(evt->req.body(), nullptr, false, true);
    if (req.is_discarded())
        return send_http_resp(http, evt, 501, resp("Failed to parse JSON"));

    const auto& accId = req["id"];
    if (!accId.is_number())
        return send_http_resp(http, evt, 400, resp("Wrong account id"));

    int id = accId.get<int>();
    auto it = workersById.find(id);
    if (it == workersById.end())
        return send_http_resp(http, evt, 404, resp("Account not found"));

    so_5::send<IRCWorker::Reload>(it->second->so_direct_mbox(), db->loadAccount(id));

    logger->logTrace("IRCController Reload account(id={})", id);

    json body = {{"id", id}, {"result", "Account reloaded"}};
    send_http_resp(http, evt, 200, body.dump());
}

void IRCController::evtHttpAccountStats(mhood_t<hreq::irc::account::stats> evt) {

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
        for (auto &[nick, worker]: workersByName)
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

                    auto it = workersByName.find(account.get<std::string_view>());
                    if (it != workersByName.end())
                        fillAccount(body[it->first], it->second->getStats());
                }
                break;
            case json::value_t::string: {
                auto it = workersByName.find(accounts.get<std::string_view>());
                if (it != workersByName.end())
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
        for (auto &[nick, worker]: workersByName)
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

                    auto it = workersByName.find(account.get<std::string_view>());
                    if (it != workersByName.end())
                        body[it->first] = json(it->second->getJoinedChannels());
                }
                break;
            case json::value_t::string:{
                auto it = workersByName.find(accounts.get<std::string_view>());
                if (it != workersByName.end())
                    body[it->first] = json(it->second->getJoinedChannels());
                break;
            }
            default:
                break;
        }
    }
    return body.dump();
}
