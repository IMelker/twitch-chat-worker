//
// Created by imelker on 06.03.2021.
//

#include "IRCWorker.h"
#include <memory>

#include <fmt/format.h>

#define AUTH_ATTEMPTS_LIMIT 20

IRCWorker::IRCWorker(IRCConnectConfig conConfig, IRCClientConfig ircConfig, IRCWorkerListener *listener)
    : listener(listener), conConfig(std::move(conConfig)), ircConfig(std::move(ircConfig)) {
    thread = std::thread(&IRCWorker::run, this);
}

IRCWorker::~IRCWorker() {
    if (thread.joinable())
        thread.join();
}

void IRCWorker::resetState() {

}

inline bool connect(const IRCConnectConfig& cfg, IRCClient *client) {
    int attempt = 0;
    while (attempt < cfg.connect_attemps_limit) {
        std::this_thread::sleep_for (std::chrono::seconds(2 * attempt));

        if (client->connect((char *)cfg.host.c_str(), cfg.port))
            return true;

        fprintf(stderr, "Failed to connect IRC with hostname: %s:%d, attempt: %d\n", cfg.host.c_str(), cfg.port, attempt);

        ++attempt;
    }
    return false;
}

inline bool login(const IRCClientConfig& cfg, IRCClient *client) {
    int attempts = AUTH_ATTEMPTS_LIMIT;
    while(--attempts > 0 && client->connected()) {
        if (client->login(cfg.nick, cfg.user, cfg.password))
            return true;

        fprintf(stderr, "Failed to login IRC with nick: %s, user: %s, attempts left: %d\n", cfg.nick.c_str(), cfg.user.c_str(), attempts);

        std::this_thread::sleep_for(std::chrono::milliseconds ((1000 / cfg.auth_per_sec_limit) - 1));
    }
    return false;
}

void IRCWorker::run() {
    while (!SysSignal::serviceTerminated()) {
        client = std::make_unique<IRCClient>();

        client->registerHook("PRIVMSG", [this] (IRCMessage message) {
            this->messageHook(std::move(message));
        });

        if (!connect(conConfig, client.get()))
            break;

        this->listener->onConnected(this);

        if (!login(ircConfig, client.get()))
            break;

        this->listener->onLogin(this);

        for(auto &channel: joinedChannels) {
            client->sendIRC(fmt::format("JOIN #{}\n", channel));
        }

        while (!SysSignal::serviceTerminated() && client->connected()) {
            client->receive();
        }

        listener->onDisconnected(this);

        resetState();
    }

    if (client->connected()) {
        client->disconnect();
        listener->onDisconnected(this);
    }
    resetState();
}


bool IRCWorker::joinChannel(const std::string &channel) {
    if (joinedChannels.count(channel)) {
        printf("%s: already joined to channel: %s\n", ircConfig.nick.c_str(), channel.c_str());
        return true;
    }

    if (joinedChannels.size() >= static_cast<size_t>(ircConfig.channels_limit)) {
        printf("%s: failed to join to channel: %s. Limit reached\n", ircConfig.nick.c_str(), channel.c_str());
        return false;
    }

    joinedChannels.emplace(channel);

    if (client && client->connected()) {
        client->sendIRC(fmt::format("JOIN #{}\n", channel));
    }
    return true;
}

void IRCWorker::leaveChannel(const std::string &channel) {
    if (joinedChannels.count(channel) == 0) {
        printf("%s: already left from channel: %s\n", ircConfig.nick.c_str(), channel.c_str());
        return;
    }

    joinedChannels.erase(channel);

    if (client && client->connected()) {
        client->sendIRC(fmt::format("PART #{}\n", channel));
    }
}

bool IRCWorker::sendMessage(const std::string &channel, const std::string &text) {
    if (client && client->connected())
        return client->sendIRC(fmt::format("PRIVMSG #{} {}\n", channel, text));
    return false;
}

bool IRCWorker::sendIRC(const std::string& message) {
    // 20 per 30 seconds: Users sending commands or messages to channels in which
    // they do not have Moderator or Operator status

    // 100 per 30 seconds: Users sending commands or messages to channels in which
    // they have Moderator or Operator status

    // For Whispers, which are private chat message between two users:
    // 3 per second, up to 100 per minute(40 accounts per day)

    if (client && client->connected())
        client->sendIRC(message);
}

void IRCWorker::messageHook(IRCMessage message) {
    this->listener->onMessage(std::move(message), this);
}
