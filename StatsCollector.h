//
// Created by l2pic on 07.05.2021.
//

#ifndef CHATCONTROLLER__STATSCOLLECTOR_H_
#define CHATCONTROLLER__STATSCOLLECTOR_H_

#include <so_5/agent.hpp>

#include "HttpControllerEvents.h"
#include "ChatMessage.h"

class Logger;
class StatsCollector final : public so_5::agent_t
{
    struct IRCIncomingMetrics { char b; };
    struct IRCOutgoingMetrics { char c; };
  public:
    StatsCollector(const context_t &ctx,
                  so_5::mbox_t publisher,
                  so_5::mbox_t http,
                  std::shared_ptr<Logger> logger);
    ~StatsCollector() override;

    // so_5::agent_t implementation
    void so_define_agent() override;
    void so_evt_start() override;
    void so_evt_finish() override;

    // event handlers
    void evtIRCIncomingMetrics(so_5::mhood_t<IRCIncomingMetrics> evt);
    void evtIRCOutgoingMetrics(so_5::mhood_t<IRCOutgoingMetrics> evt);
    void evtMessageMetric(so_5::mhood_t<Chat::Message> evt);

    // event http
    void evtHttpIrcBots(so_5::mhood_t<hreq::stats::bot> evt);
    void evtHttpStorageStats(so_5::mhood_t<hreq::stats::storage> evt);
    void evtHttpDbStats(so_5::mhood_t<hreq::stats::db> evt);
    void evtHttpIrcStats(so_5::mhood_t<hreq::stats::irc> evt);
    void evtHttpChannelsStats(so_5::mhood_t<hreq::stats::channel> evt);
    void evtHttpAccountsStats(so_5::mhood_t<hreq::stats::account> evt);
  private:
    so_5::mbox_t publisher;
    so_5::mbox_t http;

    const std::shared_ptr<Logger> logger;
};

#endif //CHATCONTROLLER__STATSCOLLECTOR_H_
