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

bool IRCClient::login(const std::string& nick, const std::string& user, const std::string& password) {
    this->nick = nick;
    this->user = user;

    std::lock_guard lg(mutex);
    if (!password.empty() && !sendIRC("PASS " + password + "\n"))
        return false;
    if (sendIRC("NICK " + nick + "\n"))
        if (sendIRC("USER " + user + " 8 * :" APP_NAME "-" APP_VERSION "(" APP_AUTHOR_EMAIL ") IRC Client\n"))
            return true;

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

    // message = [":" prefix SPACE] command[params] crlf
    // prefix = servername / (nickname[["!" user] "@" host])
    // command = 1 * letter / 3digit
    // params = *14(SPACE middle)[SPACE ":" trailing]
    // = / 14(SPACE middle)[SPACE[":"] trailing]
    //
    // nospcrlfcl = %x01 - 09 / %x0B - 0C / %x0E - 1F / %x21 - 39 / %x3B - FF
    // ; any octet except NUL, CR, LF, " " and ":"
    // middle = nospcrlfcl *(":" / nospcrlfcl)
    // trailing = *(":" / " " / nospcrlfcl)
    //
    // SPACE = %x20; space character
    // crlf = %x0D %x0A; "carriage return" "linefeed"
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

void IRCClient::handleCTCP(const IRCMessage&) {
    /*std::string text{message.parameters.back()};

    // Remove '\001' from start/end of the string
    text = text.substr(1, text.size() - 2);

    //printf("handleCTCP: [%s requested CTCP %s]\n", message.prefix.nickname.c_str(), text.c_str());

    if (auto to = message.parameters[0]; to == nick) {
        // Respond to CTCP VERSION
        if (text == "VERSION") {
            sendIRC(fmt::format("NOTICE {} :\001VERSION " APP_NAME "-" APP_VERSION " \001\n", message.prefix.nickname));
            return;
        }

        // CTCP not implemented
        sendIRC(fmt::format("NOTICE {} :\001ERRMSG {} :Not implemented\001\n", message.prefix.nickname, text));
    }*/
}

void IRCClient::handlePrivMsg(const IRCMessage &message) {
    // Handle Client-To-Client Protocol
    if (message.parameters.back()[0] == '\001') {
        handleCTCP(message);
        return;
    }

    listener->onMessage(message);
    /*auto to = message.parameters[0];
    auto text = message.parameters.back();

    if (to[0] == '#') {
        printf("handlePrivMsg: [%s] %s: %s\n", to.c_str(), message.prefix.nickname.c_str(), text.c_str());
    } else {
        printf("handlePrivMsg: %s: %s\n", message.prefix.nickname.c_str(), text.c_str());
    }*/
}

void IRCClient::handleNotice(const IRCMessage&) {
    /*std::string text;
    if (!message.parameters.empty())
        text = message.parameters.back();

    auto from = message.prefix.nickname.empty() ? message.prefix.prefix : message.prefix.nickname;
    if (!text.empty() && text[0] == '\001') {
        text = text.substr(1, text.size() - 2);
        if (text.find(' ') == std::string::npos) {
            fprintf(stderr, "[Invalid %s reply from %*s]\n", text.c_str(), (int)from.size(), from.data());
            return;
        }
        std::string ctcp = text.substr(0, text.find(' '));
        printf("handleNotice: [%s %s reply]: %s\n", from.c_str(), ctcp.c_str(), text.substr(text.find(' ') + 1).c_str());
    } else{
        printf("handleNotice: %s- %s\n", from.c_str(), text.c_str());
    }*/
}

void IRCClient::handleChannelJoinPart(const IRCMessage&) {
    /*auto channel = message.parameters.at(0);
    auto action = (message.command == "JOIN") ? "joins" : "leaves";
    printf("handleChannelJoinPart: %s %s %s\n", message.prefix.nickname.c_str(), action, channel.c_str());*/
}

void IRCClient::handleUserNickChange(const IRCMessage&) {
    /*auto newNick = message.parameters.at(0);
    printf("handleUserNickChange: %s changed his nick to %s\n", message.prefix.nickname.c_str(), newNick.c_str());*/
}

void IRCClient::handleUserQuit(const IRCMessage&) {
    /*auto text = message.parameters.at(0);
    printf("handleUserQuit: %s quits (%s)\n", message.prefix.nickname.c_str(), text.c_str());*/
}

void IRCClient::handleChannelNamesList(const IRCMessage&) {
    /*auto channel = message.parameters.at(2);
    auto nicks = message.parameters.at(3);
    //printf("handleChannelNamesList: [%s] people:\n%s\n", channel.c_str(), nicks.c_str());*/
}

void IRCClient::handleNicknameInUse(const IRCMessage&) {
    //printf("handleNicknameInUse: %s %s\n", message.parameters.at(1).c_str(), message.parameters.at(2).c_str());
}

void IRCClient::handleServerMessage(const IRCMessage&) {
    /*if (message.parameters.empty())
        return;

    auto it = message.parameters.begin();
    ++it; // skip the first parameter (our nick)
    for (; it != message.parameters.end(); ++it)
        printf("%s ", it->c_str());
    printf("\n");*/
}
