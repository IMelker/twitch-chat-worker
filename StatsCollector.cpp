//
// Created by l2pic on 07.05.2021.
//

#include <nlohmann/json.hpp>

#include "Clock.h"
#include "Logger.h"
#include "ThreadName.h"
#include "DBController.h"
#include "StatsCollector.h"

using json = nlohmann::json;

StatsCollector::StatsCollector(const context_t &ctx,
                             so_5::mbox_t publisher,
                             so_5::mbox_t http,
                             std::shared_ptr<Logger> logger,
                             std::shared_ptr<DBController> db)
  : so_5::agent_t(ctx),
    publisher(std::move(publisher)),
    http(std::move(http)),
    logger(std::move(logger)),
    db(std::move(db)) {
}

StatsCollector::~StatsCollector() = default;

void StatsCollector::so_define_agent() {
    so_subscribe_self().event(&StatsCollector::evtIRCMetrics);
    so_subscribe_self().event(&StatsCollector::evtIRCClientChannelsMetrics);
    so_subscribe(publisher).event(&StatsCollector::evtRecvMessageMetric);
    so_subscribe_self().event(&StatsCollector::evtSendMessageMetric);
    so_subscribe_self().event(&StatsCollector::evtCHPoolMetric);

    so_subscribe(http).event(&StatsCollector::evtHttpIrcBots);
    so_subscribe(http).event(&StatsCollector::evtHttpStorageStats);
    so_subscribe(http).event(&StatsCollector::evtHttpDbStats);
    so_subscribe(http).event(&StatsCollector::evtHttpIrcStats);
    so_subscribe(http).event(&StatsCollector::evtHttpChannelsStats);
}

void StatsCollector::so_evt_start() {
    set_thread_name("stats_collector");
}

void StatsCollector::so_evt_finish() {

}

void StatsCollector::evtIRCMetrics(mhood_t<Irc::SessionMetrics> evt) {
    allIrcStats += evt->stats;
    auto &sessionStats = ircStats[evt->nick];
    if (sessionStats.size() <= evt->id) {
        sessionStats.resize(evt->id + 1);
    }
    sessionStats[evt->id] += evt->stats;
}

void StatsCollector::evtIRCClientChannelsMetrics(so_5::mhood_t<Irc::ClientChannelsMetrics> evt) {
    ircClientChannels[evt->nick] = std::move(evt->channelsToSession);
}

void StatsCollector::evtRecvMessageMetric(so_5::mhood_t<Chat::Message> evt) {
    auto &stats = channelsStats[evt->channel];
    ++stats.in.count;
    stats.updated = CurrentTime<std::chrono::system_clock>::milliseconds();
}

void StatsCollector::evtSendMessageMetric(so_5::mhood_t<Chat::SendMessage> evt) {
    auto &stats = channelsStats[evt->channel];
    ++stats.out.count;
    stats.updated = CurrentTime<std::chrono::system_clock>::milliseconds();
}

void StatsCollector::evtCHPoolMetric(so_5::mhood_t<Storage::CHPoolMetrics> evt) {
    chPoolStats.resize(std::max(chPoolStats.size(), evt->stats.size()));
    for (size_t i = 0; i < chPoolStats.size(); ++i) {
        if (i >= evt->stats.size())
            break;
        chPoolStats[i] += evt->stats[i];
    }
}

void StatsCollector::evtHttpIrcBots(so_5::mhood_t<hreq::stats::bot> evt) {
    //json req = json::parse(evt->req.body(), nullptr, false, true);
    //if (req.is_discarded())
    //    return send_http_resp(http, evt, 400, resp("Failed to parse JSON"));

    /*std::string channelName;
    ChannelStatistic stats;
    {
        std::lock_guard lg(ircClientsMutex);
        for(auto &[id, client] : ircClientsById) {
            stats += client->getChannelStatic(channelName);
        }
    }*/
    //json body = json::object();
    //body[channelName] = statsToJson(stats);
    //send_http_resp(http, evt, 200, body.dump());
    send_http_resp(http, evt, 200, resp(__PRETTY_FUNCTION__));
}

void StatsCollector::evtHttpStorageStats(so_5::mhood_t<hreq::stats::storage> evt) {
    auto body = json::object();
    auto& conns = body["connections"] = json::array();
    for(size_t i = 0; i < chPoolStats.size(); ++i) {
        const auto &stats = chPoolStats[i];
        conns.push_back({{"id", i},
                         {"count", stats.count},
                         {"rows", stats.rows},
                         {"failed", stats.failed},
                         {"rtt", stats.rtt}});
    }
    chPoolStats.clear();

    send_http_resp(http, evt, 200, body.dump());
}

void StatsCollector::evtHttpDbStats(so_5::mhood_t<hreq::stats::db> evt) {
    send_http_resp(http, evt, 200, db->getStats());
}

void StatsCollector::evtHttpIrcStats(so_5::mhood_t<hreq::stats::irc> evt) {
    auto dumpAccountWithSessions = [this] (const std::vector<IRCStatistic>& stats, const std::string& nick) {
        const auto& channels = ircClientChannels[nick];

        json res = json::object();
        res["nick"] = nick;
        auto &sessions = res["sessions"] = json::array();
        bool hasUnattached = false;
        for (size_t i = 0; i < stats.size(); ++i) {
            auto session = ircStatisticToJson(stats[i]);
            auto &joinedTo = session["channels"] = json::array();
            for (auto &[name, id]: channels) {
                if (id == i)
                    joinedTo.push_back(name);
                if (id == -1u)
                    hasUnattached = true;
            }
            session["id"] = i;
            sessions.push_back(std::move(session));
        }

        if (hasUnattached) {
            auto& unattached = res["unattached_channels"] = json::array();
            for (auto &[name, id]: channels) {
                if (id == -1u)
                    unattached.push_back(name);
            }
        }

        return res;
    };

    json body = json::object();
    if (evt->req.body().empty()) {
        body = ircStatisticToJson(allIrcStats);
        allIrcStats.clear();
    } else {
        json req = json::parse(evt->req.body(), nullptr, false, true);
        if (req.is_discarded())
            return send_http_resp(http, evt, 400, resp("Failed to parse JSON"));

        const auto &accounts = req["accounts"];
        if (!accounts.is_array())
            return send_http_resp(http, evt, 400, resp("Invalid accounts type"));

        auto& clients = body["accounts"] = json::array();
        if (accounts.empty()) {
            for (auto &[nick, stats]: ircStats)
                clients.push_back(dumpAccountWithSessions(stats, nick));
            ircStats.clear();
        } else {
            for (const auto &account: accounts) {
                auto nick = account.get<std::string>();
                auto it = ircStats.find(nick);
                if (it != ircStats.end()) {
                    clients.push_back(dumpAccountWithSessions(it->second, nick));
                    ircStats.erase(it);
                } else {
                    clients.push_back({"nick", nick});
                }
            }
        }
    }

    send_http_resp(http, evt, 200, body.dump());
}

void StatsCollector::evtHttpChannelsStats(so_5::mhood_t<hreq::stats::channel> evt) {
    auto statsToJson = [] (const std::string& name, auto&& stats) {
        json channel = json::object();
        channel["name"] = name;
        channel["in"]["count"] = stats.in.count;
        channel["out"]["count"] = stats.out.count;
        channel["updated"] = stats.updated;
        stats = {};
        return channel;
    };

    json body = json::object();
    auto &channels = body["channels"] = json::array();
    if (evt->req.body().empty()) {
        for(auto &[name, stats]: channelsStats)
            channels.push_back(statsToJson(name, stats));
        channelsStats.clear();
    } else {
        json req = json::parse(evt->req.body(), nullptr, false, true);
        if (req.is_discarded())
            return send_http_resp(http, evt, 400, resp("Failed to parse JSON"));

        const auto &list = req["channels"];
        if (!list.is_array())
            return send_http_resp(http, evt, 400, resp("Invalid channels type"));

        for (const auto &channel: list) {
            auto chan = channel.get<std::string>();
            auto it = channelsStats.find(chan);
            if (it != channelsStats.end()) {
                channels.push_back(statsToJson(chan, it->second));
                channelsStats.erase(it);
            } else {
                channels.push_back(statsToJson(chan, ChannelStats{}));
            }
        }
    }

    send_http_resp(http, evt, 200, body.dump());
}
