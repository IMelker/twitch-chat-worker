//
// Created by imelker on 01.04.2021.
//

#include "nlohmann/json.hpp"

#include <so_5/send_functions.hpp>

#include "Logger.h"
#include "Clock.h"

#include "../DBController.h"
#include "IRCController.h"
#include "IRCStatistic.h"

#define resp(str) json{{"result", str}}.dump()

using json = nlohmann::json;

IRCController::IRCController(const context_t &ctx,
                             so_5::mbox_t processor,
                             so_5::mbox_t http,
                             const IRCConnectionConfig &conConfig,
                             std::shared_ptr<DBController> db,
                             std::shared_ptr<Logger> logger)
  : so_5::agent_t(ctx), processor(std::move(processor)), http(std::move(http)),
    logger(logger), db(std::move(db)), config(conConfig), pool(logger) {

}

IRCController::~IRCController() = default;

void IRCController::so_define_agent() {
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
    pool.init(config.threads);

    auto accounts = this->db->loadAccounts();
    for (auto &account : accounts)
        addNewIrcClient(account);

    this->logger->logInfo("IRCController {} clients inited", ircClientsByName.size());
}

void IRCController::so_evt_finish() {
}

void IRCController::addNewIrcClient(const IRCClientConfig& cliConfig) {
    auto *ircClient = so_5::introduce_child_coop(*this, [&cliConfig, this] (so_5::coop_t &coop) {
        return coop.make_agent<IRCClient>(so_direct_mbox(), processor, config, cliConfig, &pool, logger, db);
    });

    ircClientsByName.emplace(cliConfig.nick, ircClient);
    ircClientsById.emplace(cliConfig.id, ircClient);
}

void IRCController::evtSendMessage(mhood_t<Irc::SendMessage> message) {
    auto it = ircClientsByName.find(message->account);
    if (it != ircClientsByName.end()) {
        so_5::send(it->second->so_direct_mbox(), message);
    } else {
        logger->logWarn("Failed to find IRC worker for account: ", message->account);
    }
}

void IRCController::evtHttpStats(mhood_t<hreq::irc::stats> evt) {
    json body = json::object();
    for (auto &[name, client]: ircClientsByName) {
        body[name] = statsToJson(client->statistic());
    }
    send_http_resp(http, evt, 200, body.dump());
}

void IRCController::evtHttpReload(mhood_t<hreq::irc::reload> evt) {
    json body = json::object();

    /*for (auto &[name, client]: ircClientsByName) {
        body[name] = true;
        so_5::send<IRCClient::Reload>(client->so_direct_mbox());
    }*/

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
            auto it = ircClientsByName.find(acc);
            if (it == ircClientsByName.end()) {
                body[acc] = false;
                continue;
            }

            logger->logInfo("IRCController[{}] IRCClient({}/{}) HTTP send command: \"{}\"",
                            fmt::ptr(this), fmt::ptr(it->second), acc, data);
            so_5::send<Irc::SendIRC>(it->second->so_direct_mbox(), data);
            body[acc] = true;
        } else if (account.is_number()) {
            int id = account.get<int>();
            auto it = ircClientsById.find(id);
            if (it == ircClientsById.end()) {
                body[id] = false;
                continue;
            }

            logger->logInfo("IRCController[{}] IRCClient({}/{}) HTTP send command: \"{}\"",
                            fmt::ptr(this), fmt::ptr(it->second), it->second->nickname(), data);
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

    IRCClient *client = nullptr;
    const auto& account = req["account"];
    if (account.is_string()) {
        auto it = ircClientsByName.find(account.get<std::string_view>());
        if (it == ircClientsByName.end())
            return send_http_resp(http, evt, 404, resp("Account not found"));
        client = it->second;
    } else if (account.is_number()) {
        auto it = ircClientsById.find(account.get<int>());
        if (it == ircClientsById.end())
            return send_http_resp(http, evt, 404, resp("Account not found"));
        client = it->second;
    } else {
        return send_http_resp(http, evt, 400, resp("Invalid accounts type"));
    }

    json body = json::object();
    if (client) {
        const auto &channels = req["channels"];
        if (!channels.is_array())
            return send_http_resp(http, evt, 400, resp("Invalid channels type"));

        for (const auto &channel: channels) {
            if (!channel.is_string())
                continue;

            auto chan = channel.get<std::string>();
            so_5::send<Irc::JoinChannel>(client->so_direct_mbox(), chan);
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
        for (auto &[name, client]: ircClientsByName) {
            so_5::send<Irc::LeaveChannel>(client->so_direct_mbox(), chan);
        }
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
        IRCClient *client = nullptr;
        if (account.is_string()) {
            const auto& acc = account.get<std::string>();
            auto it = ircClientsByName.find(acc);
            if (it == ircClientsByName.end())
                continue;
            client = it->second;
        } else if (account.is_number()) {
            int id = account.get<int>();
            auto it = ircClientsById.find(id);
            if (it == ircClientsById.end())
                continue;
            client = it->second;
        }

        if (client) {
            auto nick = client->nickname();
            auto msg = text.get<std::string>();
            auto accountBody = body[nick];
            for (const auto &channel: channels) {
                auto chan = channel.get<std::string>();
                logger->logInfo(R"(IRCWorker[{}] {} send message: "{}")", fmt::ptr(client), chan, msg);
                so_5::send<Irc::SendMessage>(client->so_direct_mbox(), nick, chan, msg);
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
    auto it = ircClientsById.find(id);
    if (it != ircClientsById.end())
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

    addNewIrcClient(account);

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
    auto it = ircClientsById.find(id);
    if (it == ircClientsById.end())
        return send_http_resp(http, evt, 404, resp("Account not found"));

    ircClientsByName.erase(it->second->nickname());
    so_5::send<Irc::Shutdown>(it->second->so_direct_mbox());
    ircClientsById.erase(it);

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
    auto it = ircClientsById.find(id);
    if (it == ircClientsById.end())
        return send_http_resp(http, evt, 404, resp("Account not found"));

    so_5::send<IRCClient::Reload>(it->second->so_direct_mbox(), db->loadAccount(id));

    logger->logTrace("IRCController Reload account(id={})", id);

    json body = {{"id", id}, {"result", "Account reloaded"}};
    send_http_resp(http, evt, 200, body.dump());
}

void IRCController::evtHttpAccountStats(mhood_t<hreq::irc::account::stats> evt) {
    json req = json::parse(evt->req.body(), nullptr, false, true);
    if (req.is_discarded())
        return send_http_resp(http, evt, 501, resp("Failed to parse JSON"));

    json body = json::object();


    send_http_resp(http, evt, 200, body.dump());
}
