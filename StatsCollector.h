//
// Created by l2pic on 07.05.2021.
//

#ifndef CHATCONTROLLER__STATSCOLLECTOR_H_
#define CHATCONTROLLER__STATSCOLLECTOR_H_

#include <string>
#include <vector>
#include <map>

#include <so_5/agent.hpp>
#include <so_5/stats/messages.hpp>
#include <so_5/stats/prefix.hpp>

#include "HttpControllerEvents.h"
#include "ChatMessage.h"
#include "Storage.h"
#include "irc/IRCStatistic.h"


struct ChannelStats {
    struct {
        int count = 0;
    } in;
    struct {
        int count = 0;
    } out;
    long long updated = 0;
};

struct So5DispatcherStats {
    size_t size = 0;
};

class Logger;
class DBController;
class StatsCollector final : public so_5::agent_t
{
  public:
    StatsCollector(const context_t &ctx,
                  so_5::mbox_t publisher,
                  so_5::mbox_t http,
                  std::shared_ptr<Logger> logger,
                  std::shared_ptr<DBController> db);
    ~StatsCollector() override;

    // so_5::agent_t implementation
    void so_define_agent() override;
    void so_evt_start() override;
    void so_evt_finish() override;

    // event handlers
    void evtQuantity(const so_5::stats::messages::quantity<std::size_t> &evt);
    void evtIRCMetrics(so_5::mhood_t<Irc::SessionMetrics> evt);
    void evtIRCClientChannelsMetrics(so_5::mhood_t<Irc::ClientChannelsMetrics> evt);
    void evtRecvMessageMetric(so_5::mhood_t<Chat::Message> evt);
    void evtSendMessageMetric(so_5::mhood_t<Chat::SendMessage> evt);
    void evtCHPoolMetric(so_5::mhood_t<Storage::CHPoolMetrics> evt);

    // event http
    void evtHttpSo5Disp(so_5::mhood_t<hreq::stats::so5disp> evt);
    void evtHttpIrcBots(so_5::mhood_t<hreq::stats::bot> evt);
    void evtHttpStorageStats(so_5::mhood_t<hreq::stats::storage> evt);
    void evtHttpDbStats(so_5::mhood_t<hreq::stats::db> evt);
    void evtHttpIrcStats(so_5::mhood_t<hreq::stats::irc> evt);
    void evtHttpAccountsStats(so_5::mhood_t<hreq::stats::account> evt);
    void evtHttpChannelsStats(so_5::mhood_t<hreq::stats::channel> evt);
  private:
    so_5::mbox_t publisher;
    so_5::mbox_t http;

    const std::shared_ptr<Logger> logger;
    const std::shared_ptr<DBController> db;

    IRCStatistic allIrcStats;
    std::map<std::string, Irc::ChannelsToSessionId> ircClientChannels;
    std::map<std::string, std::vector<IRCStatistic>> ircStats;
    std::map<std::string, ChannelStats> channelsStats;
    std::vector<CHConnection::CHStatistics> chPoolStats;
    std::map<so_5::stats::prefix_t, So5DispatcherStats> dispStats;
};

#endif //CHATCONTROLLER__STATSCOLLECTOR_H_
