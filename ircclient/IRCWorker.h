//
// Created by l2pic on 06.03.2021.
//

#ifndef CHATSNIFFER_IRCCLIENT_IRCWORKER_H_
#define CHATSNIFFER_IRCCLIENT_IRCWORKER_H_

#include <thread>
#include <mutex>
#include <utility>
#include <vector>

#include "../common/SysSignal.h"

#include "IRCClient.h"

struct IRCWorkerListener {
    virtual void onConnected(IRCClient *client) = 0;
    virtual void onDisconnected(IRCClient *client) = 0;
    virtual void onMessage(IRCMessage message, IRCClient *client) = 0;
    virtual void onLogin(IRCClient *client) = 0;
};

class IRCWorker
{
  public:
    explicit IRCWorker(std::string host, int port,
                       std::string nick, std::string user, std::string pass,
                       IRCWorkerListener *listener);
    ~IRCWorker();

    IRCWorker(IRCWorker&) = delete;
    IRCWorker(IRCWorker&&) = default;
    IRCWorker& operator=(IRCWorker&) = delete;
    IRCWorker& operator=(IRCWorker&&) = default;

    void run();

    void sendIRC(const std::string& message);

    void messageHook(IRCMessage message);
  private:
    std::unique_ptr<IRCClient> client;
    int attemps = 0;

    std::string host;
    int port;

    std::string nick, user, password;

    IRCWorkerListener *listener;

    std::thread thread;
};

#endif //CHATSNIFFER_IRCCLIENT_IRCWORKER_H_
