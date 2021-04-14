//
// Created by imelker on 01.04.2021.
//

#ifndef CHATSNIFFER_IRC_IRCWORKERPOOL_H_
#define CHATSNIFFER_IRC_IRCWORKERPOOL_H_

#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <string>

#include <so_5/agent.hpp>

#include "../DBController.h"

#include "IRCWorker.h"
#include "IRCEvents.h"

#include "../http/server/HTTPServerUnit.h"

class Logger;
class IRCWorkerPool final : public so_5::agent_t, public HTTPServerUnit
{
  public:
    IRCWorkerPool(const context_t &ctx, so_5::mbox_t processor,
                  const IRCConnectConfig &conConfig, DBController *db, std::shared_ptr<Logger> logger);
    ~IRCWorkerPool() override;

    // implementation so_5::agent_t
    void so_define_agent() override;
    void so_evt_start() override;
    void so_evt_finish() override;

    // so_5 events
    void evtSendMessage(mhood_t<SendMessage> message);
    void evtWorkerConnected(mhood_t<WorkerConnected> evt);
    void evtWorkerDisconnected(mhood_t<WorkerDisconnected> evt);
    void evtWorkerLogin(mhood_t<WorkerLoggedIn> evt);

    // implementation HTTPServerUnit
    std::tuple<int, std::string> processHttpRequest(std::string_view path, const std::string& request,
                                                    std::string &error) override;

    // http handlers
    std::string handleAccounts(const std::string &request, std::string &error);
    std::string handleChannels(const std::string &request, std::string &error);
    std::string handleJoin(const std::string &request, std::string &error);
    std::string handleLeave(const std::string &request, std::string &error);
    std::string handleReloadChannels(const std::string &request, std::string &error);
    std::string handleMessage(const std::string &request, std::string &error);
    std::string handleCustom(const std::string &request, std::string &error);
  private:
    so_5::mbox_t processor;
    std::shared_ptr<Logger> logger;

    const IRCConnectConfig config;
    DBController *db;

    std::map<std::string, IRCWorker *, std::less<>> workers;

    long long lastChannelLoadTimestamp = 0;
    std::map<std::string, IRCWorker *> watchChannels;
};

#endif //CHATSNIFFER_IRC_IRCWORKERPOOL_H_
