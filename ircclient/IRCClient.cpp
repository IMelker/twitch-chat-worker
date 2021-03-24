#include <iostream>
#include <algorithm>

#include "IRCSocket.h"
#include "IRCClient.h"
#include "IRCHandler.h"

#include <sstream>

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
            parse(std::string{buf + pos, static_cast<size_t>(i - pos)});
            pos = i + 1;
        }
    }
}

void IRCClient::parse(std::string data) {
    if (data.empty())
        return;

    IRCCommandPrefix cmdPrefix;

    // if command has prefix
    if (data.substr(0, 1) == ":") {
        cmdPrefix.parse(data);
        data = data.substr(data.find(' ') + 1);
    }

    auto command = data.substr(0, data.find(' '));
    std::transform(command.begin(), command.end(), command.begin(), towupper);
    if (data.find(' ') != std::string::npos)
        data = data.substr(data.find(' ') + 1);
    else
        data = "";

    std::vector<std::string> parameters;

    if (!data.empty()) {
        if (data.substr(0, 1) == ":")
            parameters.push_back(data.substr(1));
        else {
            size_t pos1 = 0, pos2;
            while ((pos2 = data.find(' ', pos1)) != std::string::npos) {
                parameters.push_back(data.substr(pos1, pos2 - pos1));
                pos1 = pos2 + 1;
                if (data.substr(pos1, 1) == ":") {
                    parameters.push_back(data.substr(pos1 + 1));
                    break;
                }
            }
            if (parameters.empty())
                parameters.push_back(data);
        }
    }

    if (command == "ERROR") {
        disconnect();
        return;
    }

    if (command == "PING") {
        sendIRC("PONG :" + parameters.at(0) + "\n");
        return;
    }

    IRCMessage ircMessage(command, cmdPrefix, parameters);

    // Default handler
    int commandIndex = getCommandHandler(command);
    if (commandIndex < NUM_IRC_CMDS) {
        IRCCommandHandler &cmdHandler = ircCommandTable[commandIndex];
        (this->*cmdHandler.handler)(ircMessage);
    }

    callHook(command, ircMessage);
}

void IRCClient::registerHook(std::string command, IRCCommandHook hook) {
    hooks.emplace(std::pair(std::move(command), std::move(hook)));
}

void IRCClient::callHook(const std::string& command, const IRCMessage& message) {
    if (hooks.empty())
        return;

    auto range = hooks.equal_range(command);
    for (auto i = range.first; i != range.second; ++i) {
        i->second(message);
    }
}
