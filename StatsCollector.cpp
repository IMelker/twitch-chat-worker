//
// Created by l2pic on 07.05.2021.
//

#include "Logger.h"
#include "StatsCollector.h"

StatsCollector::StatsCollector(const context_t &ctx,
                             so_5::mbox_t publisher,
                             so_5::mbox_t http,
                             std::shared_ptr<Logger> logger)
  : so_5::agent_t(ctx),
    publisher(std::move(publisher)),
    http(std::move(http)),
    logger(std::move(logger)) {

}

StatsCollector::~StatsCollector() = default;

void StatsCollector::so_define_agent() {
    so_subscribe(publisher).event(&StatsCollector::evtMessageMetric);

    so_subscribe_self().event(&StatsCollector::evtIRCIncomingMetrics);
    so_subscribe_self().event(&StatsCollector::evtIRCOutgoingMetrics);

    so_subscribe(http).event(&StatsCollector::evtHttpIrcBots);
    so_subscribe(http).event(&StatsCollector::evtHttpStorageStats);
    so_subscribe(http).event(&StatsCollector::evtHttpDbStats);
    so_subscribe(http).event(&StatsCollector::evtHttpIrcStats);
    so_subscribe(http).event(&StatsCollector::evtHttpChannelsStats);
    so_subscribe(http).event(&StatsCollector::evtHttpAccountsStats);
}

void StatsCollector::so_evt_start() {

}

void StatsCollector::so_evt_finish() {

}

void StatsCollector::evtIRCIncomingMetrics(so_5::mhood_t<IRCIncomingMetrics> evt) {

}

void StatsCollector::evtIRCOutgoingMetrics(so_5::mhood_t<IRCOutgoingMetrics> evt) {

}

void StatsCollector::evtMessageMetric(so_5::mhood_t<Chat::Message> evt) {

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

void StatsCollector::evtHttpDbStats(so_5::mhood_t<hreq::stats::db> evt) {
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

void StatsCollector::evtHttpIrcStats(so_5::mhood_t<hreq::stats::irc> evt) {
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

void StatsCollector::evtHttpChannelsStats(so_5::mhood_t<hreq::stats::channel> evt) {
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

void StatsCollector::evtHttpAccountsStats(so_5::mhood_t<hreq::stats::account> evt) {
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
