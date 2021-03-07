//
// Created by imelker on 06.03.2021.
//

#include "IRCWorker.h"

#include <memory>
#include <cmath>

#define MAX_CONNECT_ATTEMPS 30

IRCWorker::IRCWorker(std::string host, int port,
                     std::string nick, std::string user, std::string pass,
                     IRCWorkerListener *listener)
    : host(std::move(host)), port(port),
      nick(std::move(nick)), user(std::move(user)), password(std::move(pass)),
      listener(listener) {
    thread = std::thread(&IRCWorker::run, this);
}

IRCWorker::~IRCWorker() {
    if (thread.joinable())
        thread.join();
}

void IRCWorker::run() {
    while (!SysSignal::serviceTerminated()) {
        client = std::make_unique<IRCClient>();

        client->registerHook("PRIVMSG", [this] (IRCMessage message) {
            this->messageHook(std::move(message));
        });

        printf("IRCClient connecting\n");
        if (client->connect((char *)host.c_str(), port)) {
            attemps = 0;

            this->listener->onConnected(client.get());

            // 20 authenticate attempts per 10 seconds per user (200 for verified bots)
            if (!client->login(nick, user, password)) {
                fprintf(stderr, "Failed to login IRC with nick: %s, user: %s\n", nick.c_str(), user.c_str());
                break;
            }

            this->listener->onLogin(client.get());

            while (!SysSignal::serviceTerminated() && client->connected()) {
                client->receive();
            }
            printf("IRCClient disconnected\n");
        } else {
            if (attemps > MAX_CONNECT_ATTEMPS)
                break;

            std::this_thread::sleep_for (std::chrono::seconds(2 * attemps));
            ++attemps;
        }
    }

    if (client->connected()) {
        client->disconnect();
        listener->onDisconnected(client.get());
    }
}

void IRCWorker::sendIRC(const std::string& message) {
    // 20 per 30 seconds: Users sending commands or messages to channels in which
    // they do not have Moderator or Operator status

    // 100 per 30 seconds: Users sending commands or messages to channels in which
    // they have Moderator or Operator status

    // For Whispers, which are private chat message between two users:
    // 3 per second, up to 100 per minute(40 accounts per day)

    if (client && client->connected())
        client->sendIRC(message);

    // [JOIN](#join)	Join a channel.
    // [PART](#part)	Leave a channel.
    // [PRIVMSG](#privmsg)	Send a message to a channel
}

void IRCWorker::messageHook(IRCMessage message) {
    //std::string to = message.parameters.at(0);
    //std::string text = message.parameters.at(message.parameters.size() - 1);

    /*if (to[0] == '#') {
        printf("[%s] %s: %s\n", to.c_str(), message.prefix.nickname.c_str(), text.c_str());
    } else {
        printf("< %s: %s\n", message.prefix.nickname.c_str(), text.c_str());
    }*/

    this->listener->onMessage(std::move(message), client.get());
}
