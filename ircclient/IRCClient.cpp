#include <iostream>
#include <algorithm>

#include "IRCSocket.h"
#include "IRCClient.h"
#include "IRCHandler.h"

#define MAXDATASIZE 4096

bool IRCClient::connect(const char *host, int port) {
    std::lock_guard lg(mutex);
    return this->ircSocket.connect(host, port);
}

void IRCClient::disconnect() {
    std::lock_guard lg(mutex);
    this->ircSocket.disconnect();
}

bool IRCClient::sendIRC(const std::string& data) {
    std::lock_guard lg(mutex);
    return this->ircSocket.send(data.c_str(), data.size());
}

bool IRCClient::login(const std::string& nick, const std::string& user, const std::string& password) {
    this->nick = nick;
    this->user = user;

    std::lock_guard lg(mutex);
    if (sendIRC("HELLO\n")) {
        if (!password.empty() && !sendIRC("PASS " + password + "\n"))
            return false;
        if (sendIRC("NICK " + nick + "\n"))
            if (sendIRC("USER " + user + " 8 * :Cpp IRC Client\n"))
                return true;
    }

    return false;
}

void IRCClient::receive() {
    char buf[MAXDATASIZE];
    int size = this->ircSocket.receive(buf, MAXDATASIZE);
    if (size <= 0)
        return;

    int pos = 0;
    for (int i = 0; i < size; ++i) {
        if (buf[i] == '\r' || buf[i] == '\n') {
            process(buf + pos, i - pos);
            pos = i + 1;
        }
    }
}

void IRCClient::process(const char* data, size_t len) {
    if (len == 0)
        return;

    IRCMessage message{data, len};

    if (message.command == "ERROR") {
        disconnect();
        return;
    }

    if (message.command == "PING") {
        std::string pong;
        pong.reserve(message.parameters[0].size() + sizeof("PONG :\n"));
        pong.append("PONG :").append(message.parameters.at(0)).append("\n");
        sendIRC(pong);
        return;
    }

    // Default handler
    int commandIndex = getCommandHandler(message.command);
    if (commandIndex < NUM_IRC_CMDS) {
        IRCCommandHandler &cmdHandler = ircCommandTable[commandIndex];
        (this->*cmdHandler.handler)(message);
    }

    callHook(message.command, message);
}

void IRCClient::registerHook(std::string command, IRCCommandHook hook) {
    hooks.emplace(std::pair(std::move(command), std::move(hook)));
}

void IRCClient::callHook(const std::string & command, const IRCMessage& message) {
    if (hooks.empty())
        return;

    auto range = hooks.equal_range(command);
    for (auto i = range.first; i != range.second; ++i) {
        i->second(message);
    }
}
