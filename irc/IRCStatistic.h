//
// Created by l2pic on 27.04.2021.
//

#ifndef IRCTEST__IRCSTATISTIC_H_
#define IRCTEST__IRCSTATISTIC_H_

#include <vector>

#include "nlohmann/json.hpp"

using json = nlohmann::json;


struct IRCStatistic
{
    void clear() { *this = {}; }

    inline IRCStatistic& operator+(const IRCStatistic& rhs) noexcept {
        connects.success += rhs.connects.success;
        connects.failed += rhs.connects.failed;
        connects.loggedin += rhs.connects.loggedin;
        commands.in.bytes += rhs.commands.in.bytes;
        commands.in.count += rhs.commands.in.count;
        commands.out.bytes += rhs.commands.out.bytes;
        commands.out.join += rhs.commands.out.join;
        commands.out.part += rhs.commands.out.part;
        commands.out.privmsg += rhs.commands.out.privmsg;
        commands.out.count += rhs.commands.out.count;
        if (rhs.commands.ping_pong.rtt > 0) {
            commands.ping_pong.rtt = rhs.commands.ping_pong.rtt;
        }
        return *this;
    }
    inline IRCStatistic& operator+=(const IRCStatistic & rhs) {
        return operator+(rhs);
    }

    struct {
        unsigned int success = 0;
        unsigned int failed = 0;
        unsigned int loggedin = 0;
    } connects;
    struct {
        struct {
            unsigned long long bytes = 0;
            unsigned int count = 0;
        } in;
        struct {
            unsigned long long bytes = 0;
            unsigned int join = 0;
            unsigned int part = 0;
            unsigned int privmsg = 0;
            unsigned int count = 0;
        } out;
        struct {
            unsigned int rtt = 0;
        } ping_pong;
    } commands;
};

namespace Irc {
using ChannelsToSessionId = std::vector<std::pair<std::string, unsigned int>>;
struct SessionMetrics {
    unsigned int id = 0;
    std::string nick;
    IRCStatistic stats;
};
struct ClientChannelsMetrics {
    const std::string nick;
    mutable ChannelsToSessionId channelsToSession;
};
}


inline json ircStatisticToJson(const IRCStatistic& stats) {
    json res = json::object();
    res["connects"] = {
        {"success", stats.connects.success},
        {"failed", stats.connects.failed},
        {"loggedin", stats.connects.loggedin}
    };
    res["commands"] = {
        { "in", {
            {"bytes", stats.commands.in.bytes},
            {"count", stats.commands.in.count}
        }
        },
        { "out", {
            {"bytes", stats.commands.out.bytes},
            {"join", stats.commands.out.join},
            {"part", stats.commands.out.part},
            {"privmsg", stats.commands.out.privmsg},
            {"count", stats.commands.out.count}
        }
        },
        { "ping_pong", {
            {"rtt", stats.commands.ping_pong.rtt}
        }
        }
    };
    return res;
};

#endif //IRCTEST__IRCSTATISTIC_H_
