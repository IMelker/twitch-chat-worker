//
// Created by imelker on 06.03.2021.
//

#ifndef CHATSNIFFER_IRCCLIENT_IRCWORKER_H_
#define CHATSNIFFER_IRCCLIENT_IRCWORKER_H_

#include <utility>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>

#include "../common/SysSignal.h"

#include "IRCClient.h"
#include "IRCMessageListener.h"

class IRCWorker;
struct IRCWorkerListener : public IRCMessageListener {
    virtual void onConnected(IRCWorker *worker) = 0;
    virtual void onDisconnected(IRCWorker *worker) = 0;
    virtual void onLogin(IRCWorker *worker) = 0;
};

struct IRCConnectConfig {
    std::string host;
    int port = 6667;
    int connect_attemps_limit = 30;
};

struct IRCClientConfig {
    std::string nick;
    std::string user;
    std::string password;
    int channels_limit = 20;
    int command_per_sec_limit = 20 / 30;
    int whisper_per_sec_limit = 3;
    int auth_per_sec_limit = 2;
};

class Logger;
class IRCWorker : public IRCMessageListener
{
    struct {
        struct {
            std::atomic<long long> updated{};
            std::atomic<unsigned int> attempts{};
        } connects;
        struct {
            std::atomic<long long> updated{};
            std::atomic<unsigned int> count{};
        } channels;
        struct {
            struct {
                std::atomic<long long> updated{};
                std::atomic<unsigned int> count{};
            } in;
            struct {
                std::atomic<long long> updated{};
                std::atomic<unsigned int> count{};
            } out;
        } messages;
    } stats;
  public:
    explicit IRCWorker(IRCConnectConfig conConfig,
                       IRCClientConfig ircConfig,
                       IRCWorkerListener *listener,
                       std::shared_ptr<Logger> logger);
    ~IRCWorker();

    IRCWorker(IRCWorker&) = delete;
    IRCWorker& operator=(IRCWorker&) = delete;

    [[nodiscard]] const IRCConnectConfig& getConnectConfig() const;
    [[nodiscard]] const IRCClientConfig& getClientConfig() const;
    [[nodiscard]] std::set<std::string> getJoinedChannels() const;
    [[nodiscard]] const decltype(stats)& getStats() const;

    bool channelLimitReached() const;

    bool joinChannel(const std::string &channel, std::string &result);
    void leaveChannel(const std::string& channel);
    bool sendMessage(const std::string& channel, const std::string& text);
    bool sendIRC(const std::string& message);

    void onMessage(const IRCMessage &message) override;
  private:
    void run();

    const std::shared_ptr<Logger> logger;
    IRCWorkerListener * const listener;

    const IRCConnectConfig conConfig;
    const IRCClientConfig ircConfig;

    std::unique_ptr<IRCClient> client;

    mutable std::mutex channelsMutex;
    std::set<std::string> joinedChannels;

    std::thread thread;
};

#endif //CHATSNIFFER_IRCCLIENT_IRCWORKER_H_
