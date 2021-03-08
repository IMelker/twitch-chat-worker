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

class IRCWorker;
struct IRCWorkerListener {
    virtual void onConnected(IRCWorker *worker) = 0;
    virtual void onDisconnected(IRCWorker *worker) = 0;
    virtual void onMessage(IRCMessage message, IRCWorker *worker) = 0;
    virtual void onLogin(IRCWorker *worker) = 0;
};

struct IRCConnectConfig {
    std::string host;
    int port = 6667;
    int connect_attemps_limit = 30;
};

struct IRCClientConfig {
    std::string user;
    std::string nick;
    std::string password;
    int channels_limit = 20;
    int command_per_sec_limit = 20 / 30;
    int whisper_per_sec_limit = 3;
    int auth_per_sec_limit = 2;
};

class IRCWorker
{
  public:
    explicit IRCWorker(IRCConnectConfig conConfig, IRCClientConfig ircConfig, IRCWorkerListener *listener);
    ~IRCWorker();

    IRCWorker(IRCWorker&) = delete;
    IRCWorker(IRCWorker&&) = default;
    IRCWorker& operator=(IRCWorker&) = delete;
    IRCWorker& operator=(IRCWorker&&) = default;

    void run();

    bool joinChannel(const std::string& channel);
    void leaveChannel(const std::string& channel);
    bool sendMessage(const std::string& channel, const std::string& text);

    bool sendIRC(const std::string& message);

    void messageHook(IRCMessage message);
  private:
    void resetState();

    IRCWorkerListener *listener;

    IRCConnectConfig conConfig;
    IRCClientConfig ircConfig;

    std::unique_ptr<IRCClient> client;
    std::set<std::string> joinedChannels;

    std::thread thread;
};

#endif //CHATSNIFFER_IRCCLIENT_IRCWORKER_H_
