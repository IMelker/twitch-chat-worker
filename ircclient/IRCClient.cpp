#include <iostream>
#include <algorithm>

#include "IRCSocket.h"
#include "IRCClient.h"
#include "IRCHandler.h"

#include <sstream>

#define MAXDATASIZE 4096

bool IRCClient::connect(char *host, int port) {
    return this->ircsocket.connect(host, port);
}

void IRCClient::disconnect() {
    this->ircsocket.disconnect();
}

bool IRCClient::sendIRC(std::string data) {
    std::cout << "> " << data << std::endl;
    data.append("\n");
    return this->ircsocket.send(data.c_str(), data.size());
}

bool IRCClient::login(const std::string& nick, const std::string& user, const std::string& password) {
    this->nick = nick;
    this->user = user;

    //if (sendIRC("HELLO")) {
        if (!password.empty() && !sendIRC("PASS " + password))
            return false;
        if (sendIRC("NICK " + nick))
            if (sendIRC("USER " + user + " 8 * :Cpp IRC Client"))
                return true;
    //}

    return false;
}

void IRCClient::receive() {
    char buf[MAXDATASIZE];
    int size = this->ircsocket.receive(buf, MAXDATASIZE);
    if (size <= 0)
        return;

    //  TODO use data from buf, to avoid copy
    std::string line;
    std::istringstream iss(std::string{buf, static_cast<size_t>(size)});
    while (getline(iss, line)) {
        if (line.find('\r') != std::string::npos)
            line = line.substr(0, line.size() - 1);
        parse(line);
    }
}

void IRCClient::parse(std::string data) {
    std::string original(data);
    IRCCommandPrefix cmdPrefix;

    // if command has prefix
    if (data.substr(0, 1) == ":") {
        cmdPrefix.parse(data);
        data = data.substr(data.find(' ') + 1);
    }

    std::string command = data.substr(0, data.find(' '));
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
        std::cout << original << std::endl;
        disconnect();
        return;
    }

    if (command == "PING") {
        sendIRC("PONG :" + parameters.at(0));
        return;
    }

    IRCMessage ircMessage(command, cmdPrefix, parameters);

    // Default handler
    int commandIndex = getCommandHandler(command);
    if (commandIndex < NUM_IRC_CMDS) {
        IRCCommandHandler &cmdHandler = ircCommandTable[commandIndex];
        (this->*cmdHandler.handler)(ircMessage);
    } else if (debug)
        std::cout << original << std::endl;

    // Try to call hook (if any matches)
    callHook(command, ircMessage);
}

void IRCClient::hookIRCCommand(std::string command, void (*function)(IRCMessage /*message*/, IRCClient * /*client*/)) {
    IRCCommandHook hook;

    hook.command = std::move(command);
    hook.function = function;

    hooks.push_back(hook);
}

void IRCClient::callHook(const std::string& command, const IRCMessage& message) {
    if (hooks.empty())
        return;

    for (auto & hook : hooks) {
        if (hook.command == command) {
            (*(hook.function))(message, this);
            break;
        }
    }
}
