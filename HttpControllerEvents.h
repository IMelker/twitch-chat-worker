//
// Created by l2pic on 15.04.2021.
//

#ifndef CHATCONTROLLER__HTTPCONTROLLEREVENTS_H_
#define CHATCONTROLLER__HTTPCONTROLLEREVENTS_H_

#include "http/server/HTTPSession.h"
#include <so_5/message.hpp>

#define DEFINE_EVT(space, name) namespace hreq::space { struct name : public base {}; }
#define DEFINE_EVT_SUB(space, subspace, name) namespace hreq::space::subspace { struct name : public base {}; }

namespace hreq
{
struct base
{
    mutable http::request<http::string_body> req;
    mutable HTTPSession::SendLambda send;
};

struct resp : public base {
    int status = 200;
    mutable std::string body;
};
}

// handled by Controller
DEFINE_EVT(app, shutdown)         // init application shutdown
DEFINE_EVT(app, version)          // get apps version

// handled by BotsEnvironment
DEFINE_EVT(bot, add)              // add bot by id from environment
DEFINE_EVT(bot, remove)           // remove bot from environment
DEFINE_EVT(bot, reload)           // reload bot configs
DEFINE_EVT(bot, reloadall)        // update configs, add new bots

// handled by Storage
DEFINE_EVT(storage, stats)         // storage stats

// handled by HttpController
DEFINE_EVT(db, stats)              // database stats

// handled by IRCController
DEFINE_EVT(irc, reload)            // reload all accounts
DEFINE_EVT(irc, stats)             // concatenated stats for all messages(rps, and etc.)

// handled by ChannelController
DEFINE_EVT_SUB(irc, channel, join)    // join to channel(for random account or exact)
DEFINE_EVT_SUB(irc, channel, leave)   // leave from channel(for all accounts)
DEFINE_EVT_SUB(irc, channel, message) // message for channel from account or random
DEFINE_EVT_SUB(irc, channel, custom)  // custom message for channel from account or random
DEFINE_EVT_SUB(irc, channel, stats)   // stats for channel, all channels

DEFINE_EVT_SUB(irc, account, add)     // add account by id from database
DEFINE_EVT_SUB(irc, account, remove)  // remove account from controller, with disconnec
DEFINE_EVT_SUB(irc, account, reload)  // reload account from database, and reconnect
DEFINE_EVT_SUB(irc, account, stats)   // stats for account, all accounts

#endif //CHATCONTROLLER__HTTPCONTROLLEREVENTS_H_
