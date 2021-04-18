#include "app.h"
#include <iostream>
#include <algorithm>

#include <fmt/format.h>

#include "../common/Logger.h"
#include "IRCSocket.h"
#include "IRCClient.h"
#include "IRCHandler.h"

IRCClient::IRCClient(IRCMessageListener *listener, std::shared_ptr<Logger> logger)
  : listener(listener), logger(std::move(logger)) {
    this->logger->logTrace("IRCClient[{}] Client created", fmt::ptr(this));
}

bool IRCClient::connect(const char *host, int port) {
    std::lock_guard lg(mutex);
    return this->ircSocket.connect(host, port);
}

void IRCClient::disconnect() {
    std::lock_guard lg(mutex);
    this->ircSocket.disconnect();
}

bool IRCClient::sendIRC(const std::string& data) {
    this->logger->logTrace("IRCClient[{}] Send \"{}\"", fmt::ptr(this), std::string_view{data.data(), data.size() - 1});
    std::lock_guard lg(mutex);
    return this->ircSocket.send(data.c_str(), data.size());
}

bool IRCClient::login(const std::string& nickname, const std::string& username, const std::string& password) {
    this->nick = nickname;
    this->user = username;

    std::lock_guard lg(mutex);
    if (!password.empty() && !sendIRC("PASS " + password + "\n"))
        return false;
    if (sendIRC("NICK " + this->nick + "\n")) {
        /*
		 * RFC 1459 states that "hostname and servername are normally
         * ignored by the IRC server when the USER command comes from
         * a directly connected client (for security reasons)". But,
         * we actually use it.
         */
        if (sendIRC("USER " + this->user + " 8 * :" APP_NAME "-" APP_VERSION "(" APP_AUTHOR_EMAIL ") IRC Client\n"))
            return true;
    }

    return false;
}

void IRCClient::receive() {
    int size = this->ircSocket.receive(buffer.getLinearAppendPtr(), static_cast<int>(buffer.getLinearFreeSpace()));
    if (size <= 0)
        return;
    buffer.expandSize(size);

    const char *buf = buffer.getDataPtr();
    unsigned int pos = 0;
    for (unsigned int i = 0; i < buffer.getSize(); ++i) {
        if (buf[i] == '\r' || buf[i] == '\n') {
            process(buf + pos, i - pos);
            pos = i + 1;
        }
    }

    if (pos == buffer.getSize())
        buffer.clear();
    else
        buffer.seekData(pos);

    if (buffer.getLinearFreeSpace() < MAX_MESSAGE_LEN)
        buffer.normilize();
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
        sendIRC(fmt::format("PONG :{}\n", message.parameters.at(0)));
        return;
    }
    if (message.command == "PONG") {
        return;
    }

    // Default handler
    int commandIndex = getCommandHandler(message.command);
    if (commandIndex < NUM_IRC_CMDS) {
        IRCCommandHandler &cmdHandler = ircCommandTable[commandIndex];
        (this->*cmdHandler.handler)(message);
    } else {
        this->logger->logWarn("IRCClient[{}] Unhandled command: {}", fmt::ptr(this), message);
        return;
    }
}

void IRCClient::handleCTCP(const IRCMessage& message) {
    // Remove '\001' from start/end of the string
    auto &text = const_cast<std::string &>(message.parameters.back());
    text = text.substr(1, text.size() - 2);

    if (text.find("ACTION") == 0) {
        text = text.substr(sizeof("ACTION"));
        listener->onMessage(message);
    }else {
        this->logger->logWarn("IRCClient[{}] Incoming CTCP: {}", fmt::ptr(this), message);
    }

    if (auto to = message.parameters[0]; to == nick) {
        // Respond to CTCP VERSION
        //if (text == "VERSION") {
        //    sendIRC(fmt::format("NOTICE {} :\001VERSION " APP_NAME "-" APP_VERSION " \001\n", message.prefix.nickname));
        //    return;
        //}

        // CTCP not implemented
        sendIRC(fmt::format("NOTICE {} :\001ERRMSG {} :Not implemented\001\n", message.prefix.nickname, text));
    }
}

void IRCClient::handlePrivMsg(const IRCMessage &message) {
    if (message.parameters.empty())
        return;

    auto &text = message.parameters.back();
    // Handle Client-To-Client Protocol
    if (text.front() == '\001' && text.back() == '\001') {
        handleCTCP(message);
        return;
    }

    listener->onMessage(message);
}

void IRCClient::handleNotice(const IRCMessage& message) {
    auto text = message.parameters.empty() ? "" : message.parameters.back();
    if (!text.empty() && text.front() == '\001' && text.back() == '\001') {
        text = text.substr(1, text.size() - 2);

        if (text.find(' ') == std::string::npos) {
            this->logger->logError("IRCClient[{}] Invalid notice ctcp reply: {}", fmt::ptr(this), message);
            return;
        }

        this->logger->logTrace("IRCClient[{}] Notice reply CTCP: {}", fmt::ptr(this), message);
    } else{
        this->logger->logTrace("IRCClient[{}] Notice: {}", fmt::ptr(this), message);
    }
}

void IRCClient::handleChannelJoinPart(const IRCMessage&) {
    /*auto channel = message.parameters.at(0);
    auto action = (message.command == "JOIN") ? "joins" : "leaves";
    printf("handleChannelJoinPart: %s %s %s\n", message.prefix.nickname.c_str(), action, channel.c_str());*/
}

void IRCClient::handleUserNickChange(const IRCMessage& message) {
    auto newNick = message.parameters.at(0);
    logger->logTrace("IRCClient[{}] Nickname changed from {} to {}", fmt::ptr(this), message.prefix.nickname, newNick);
    nick = newNick;
}

void IRCClient::handleUserQuit(const IRCMessage& message) {
    logger->logTrace("IRCClient[{}] User quit: {}", fmt::ptr(this), message);
}

void IRCClient::handleMode(const IRCMessage& message) {
    logger->logTrace("IRCClient[{}] Mode changed: {}", fmt::ptr(this), message);
}

void IRCClient::handleChannelNamesList(const IRCMessage&) {
    /*auto channel = message.parameters.at(2);
    auto nicks = message.parameters.at(3);
    //printf("handleChannelNamesList: [%s] people:\n%s\n", channel.c_str(), nicks.c_str());*/
}

void IRCClient::handleNicknameInUse(const IRCMessage&) {
    //printf("handleNicknameInUse: %s %s\n", message.parameters.at(1).c_str(), message.parameters.at(2).c_str());
}

void IRCClient::handleServerMessage(const IRCMessage& message) {
    logger->logTrace("IRCClient[{}] Server message: {}", fmt::ptr(this), message);
}
