//
// Created by l2pic on 15.04.2021.
//

#ifndef CHATCONTROLLER__HTTPCONTROLLEREVENTS_H_
#define CHATCONTROLLER__HTTPCONTROLLEREVENTS_H_

#include <string>

#include <nlohmann/json.hpp>

#include <so_5/mbox.hpp>
#include <so_5/message.hpp>
#include <so_5/send_functions.hpp>

#include "http/server/HTTPServerSession.h"

using json = nlohmann::json;

#define DEFINE_EVT(space, name) namespace hreq::space { struct name : public base {}; }
#define DEFINE_EVT_SUB(space, subspace, name) namespace hreq::space::subspace { struct name : public base {}; }

namespace hreq
{
struct base
{
    mutable http::request<http::string_body> req;
    mutable HTTPServerSession::SendLambda send;
};

struct resp : public base {
    int status = 200;
    mutable std::string body;
};
}

template<typename T>
inline std::string resp(T&& result) {
    return json{{"result", std::forward<T>(result)}}.dump();
}

template<typename T>
void send_http_resp(const so_5::mbox_t& http, T&& msg, int code, std::string body) {
    so_5::send<hreq::resp>(http, std::move(msg->req), std::move(msg->send), code, std::move(body));
};

// handled by Controller
DEFINE_EVT(app, shutdown)             // init application shutdown
DEFINE_EVT(app, version)              // get apps version

// handled by BotsEnvironment
DEFINE_EVT(bot, add)                  // add bot by id from environment
DEFINE_EVT(bot, remove)               // remove bot from environment
DEFINE_EVT(bot, reload)               // reload bot configs
DEFINE_EVT(bot, reloadall)            // update configs, add new bots

// handled by StatsCollector
DEFINE_EVT(stats, bot)                // bots stats
DEFINE_EVT(stats, storage)            // storage stats
DEFINE_EVT(stats, db)                 // db stats
DEFINE_EVT(stats, irc)                // irc stats
DEFINE_EVT(stats, account)                // irc stats
DEFINE_EVT(stats, channel)            // channels stats

// handled by IRCController
DEFINE_EVT(irc, reload)               // reload all accounts
DEFINE_EVT(irc, custom)               // custom irc message

// handled by ChannelController
DEFINE_EVT_SUB(irc, channel, join)    // join to channel(for random account or exact)
DEFINE_EVT_SUB(irc, channel, leave)   // leave from channel(for all accounts)
DEFINE_EVT_SUB(irc, channel, message) // message for channel from account or random

DEFINE_EVT_SUB(irc, account, add)     // add account by id from database
DEFINE_EVT_SUB(irc, account, remove)  // remove account from controller, with disconnect
DEFINE_EVT_SUB(irc, account, reload)  // reload account from database, and reconnect

#endif //CHATCONTROLLER__HTTPCONTROLLEREVENTS_H_
