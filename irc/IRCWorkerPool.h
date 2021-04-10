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
#include "IRCWorker.h"
#include "IRCMessageListener.h"
#include "../DBController.h"
#include "../MessageSenderInterface.h"

#include "../http/server/HTTPServerUnit.h"

class Logger;
class IRCWorkerPool : public IRCWorkerListener,
                      public HTTPServerUnit,
                      public MessageSenderInterface
{
  public:
    IRCWorkerPool(const IRCConnectConfig &conConfig, DBController *db,
                  IRCMessageListener *listener, std::shared_ptr<Logger> logger);
    ~IRCWorkerPool();

    [[nodiscard]] size_t poolSize() const;

    // implementation MessageSenderInterface
    void sendMessage(const std::string &account, const std::string &channel, const std::string &text) override;

    // implementation IRCWorkerListener
    void onConnected(IRCWorker *worker) override;
    void onDisconnected(IRCWorker *worker) override;
    void onLogin(IRCWorker *worker) override;
    void onMessage(IRCWorker *worker, const IRCMessage &message, long long now) override;

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
    IRCMessageListener *listener;
    std::shared_ptr<Logger> logger;

    DBController *db;

    std::map<std::string, std::shared_ptr<IRCWorker>, std::less<>> workers;

    std::mutex watchMutex;
    long long lastChannelLoadTimestamp = 0;
    std::map<std::string, IRCWorker *> watchChannels;
};

#endif //CHATSNIFFER_IRC_IRCWORKERPOOL_H_