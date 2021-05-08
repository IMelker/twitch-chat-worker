//
// Created by imelker on 01.04.2021.
//

#include "nlohmann/json.hpp"

#include <so_5/send_functions.hpp>

#include "Logger.h"

#include "../DBController.h"
#include "IRCController.h"
#include "IRCStatistic.h"

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
    so_subscribe_self().event(&IRCController::evtSendMessage, so_5::thread_safe);

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

    ircSendPool = so_5::disp::adv_thread_pool::make_dispatcher(so_environment(), config.threads);
    ircSendPoolParams = {};

    auto accounts = this->db->loadAccounts();
    for (auto &account : accounts)
        addNewIrcClient(account);

    this->logger->logInfo("IRCController {} clients inited", ircClientsByName.size());
}

void IRCController::so_evt_finish() {
}

void IRCController::addNewIrcClient(const IRCClientConfig& cliConfig) {
    auto *ircClient = so_5::introduce_child_coop(*this, [&cliConfig, this] (so_5::coop_t &coop) {
        return coop.make_agent_with_binder<IRCClient>(ircSendPool.binder(ircSendPoolParams),
                                                      so_direct_mbox(), processor, config, cliConfig,
                                                      &pool, logger, db);
    });

    ircClientsByName.emplace(cliConfig.nick, ircClient);
    ircClientsById.emplace(cliConfig.id, ircClient);
}

IRCClient * IRCController::getIrcClient(const std::string& name) {
    IRCClient * client = nullptr;
    if (auto it = ircClientsByName.find(name); it != ircClientsByName.end())
        client = it->second;
    return client;
}

IRCClient * IRCController::getIrcClient(int id) {
    IRCClient * client = nullptr;
    if (auto it = ircClientsById.find(id); it != ircClientsById.end())
        client = it->second;
    return client;
}

void IRCController::evtSendMessage(mhood_t<Chat::SendMessage> message) {
    auto *client = getIrcClient(message->account);
    if (!client) {
        logger->logWarn("Failed to find IRC worker for account: {}", message->account);
        return;
    }
    so_5::send<IRCClient::SendMessage>(client->so_direct_mbox(),
                                       std::move(message->channel), std::move(message->text));
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

    for (auto &[id, client]: ircClientsById) {
        auto account = db->loadAccount(id);
        body[account.nick] = true;
        so_5::send<IRCClient::Reload>(client->so_direct_mbox(), std::move(account));
    }

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
            auto *client = getIrcClient(acc);
            if (!client) {
                body[acc] = false;
                continue;
            }

            logger->logInfo("IRCController[{}] IRCClient({}/{}) HTTP send command: \"{}\"",
                            fmt::ptr(this), fmt::ptr(client), acc, data);
            so_5::send<IRCClient::SendIRC>(client->so_direct_mbox(), data);
            body[acc] = true;
        } else if (account.is_number()) {
            int id = account.get<int>();
            auto *client = getIrcClient(id);
            if (!client) {
                body[std::to_string(id)] = false;
                continue;
            }

            logger->logInfo("IRCController[{}] IRCClient({}/{}) HTTP send command: \"{}\"",
                            fmt::ptr(this), fmt::ptr(client), client->nickname(), data);
            so_5::send<IRCClient::SendIRC>(client->so_direct_mbox(), data);
            body[std::to_string(id)] = true;
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
        client = getIrcClient(account.get<std::string>());
        if (!client)
            return send_http_resp(http, evt, 404, resp("Account not found"));
    } else if (account.is_number()) {
        client = getIrcClient(account.get<int>());
        if (!client)
            return send_http_resp(http, evt, 404, resp("Account not found"));
    } else {
        return send_http_resp(http, evt, 400, resp("Invalid accounts type"));
    }

    json body = json::object();
    const auto &channels = req["channels"];
    if (!channels.is_array())
        return send_http_resp(http, evt, 400, resp("Invalid channels type"));

    for (const auto &channel: channels) {
        if (!channel.is_string())
            continue;

        auto chan = channel.get<std::string>();
        so_5::send<IRCClient::JoinChannel>(client->so_direct_mbox(), chan);
        body[chan] = {{"joined", true}};
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
            so_5::send<IRCClient::LeaveChannel>(client->so_direct_mbox(), chan);
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
            client = getIrcClient(account.get<std::string>());
            if (!client)
                continue;
        } else if (account.is_number()) {
            client = getIrcClient(account.get<int>());
            if (!client)
                continue;
        }

        if (client) {
            auto msg = text.get<std::string>();
            auto &accountBody = body[client->nickname()] = json::object();
            for (const auto &channel: channels) {
                auto chan = channel.get<std::string>();
                logger->logInfo(R"(IRCClient[{}] {} send message: "{}")", fmt::ptr(client), chan, msg);
                so_5::send<IRCClient::SendMessage>(client->so_direct_mbox(), chan, msg);
                accountBody[chan] = true;
            }
        }
    }

    send_http_resp(http, evt, 200, body.dump());
}

void IRCController::evtHttpChannelStats(mhood_t<hreq::irc::channel::stats> evt) {
    json req = json::parse(evt->req.body(), nullptr, false, true);
    if (req.is_discarded())
        return send_http_resp(http, evt, 400, resp("Failed to parse JSON"));

    /*std::string channelName;
    ChannelStatistic stats;
    {
        std::lock_guard lg(ircClientsMutex);
        for(auto &[id, client] : ircClientsById) {
            stats += client->getChannelStatic(channelName);
        }
    }*/
    json body = json::object();
    //body[channelName] = statsToJson(stats);
    send_http_resp(http, evt, 200, body.dump());
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
    auto *client = getIrcClient(id);
    if (client)
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
    so_5::send<IRCClient::Shutdown>(it->second->so_direct_mbox());
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
    auto *client = getIrcClient(id);
    if (!client)
        return send_http_resp(http, evt, 404, resp("Account not found"));

    so_5::send<IRCClient::Reload>(client->so_direct_mbox(), db->loadAccount(id));

    logger->logTrace("IRCController Reload account(id={})", id);

    json body = {{"id", id}, {"result", "Account reloaded"}};
    send_http_resp(http, evt, 200, body.dump());
}

void IRCController::evtHttpAccountStats(mhood_t<hreq::irc::account::stats> evt) {
    json req = json::parse(evt->req.body(), nullptr, false, true);
    if (req.is_discarded())
        return send_http_resp(http, evt, 501, resp("Failed to parse JSON"));

    const auto& accId = req["id"];
    if (!accId.is_number())
        return send_http_resp(http, evt, 400, resp("Wrong account id"));

    int id = accId.get<int>();
    auto *client = getIrcClient(id);
    if (!client)
        return send_http_resp(http, evt, 404, resp("Account not found"));

    json body = json::object();
    body[client->nickname()] = statsToJson(client->statistic());

    send_http_resp(http, evt, 200, body.dump());
}
