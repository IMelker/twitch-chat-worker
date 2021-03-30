#include "app.h"
#include <iostream>
#include <algorithm>

#include <fmt/format.h>

#include "IRCSocket.h"
#include "IRCClient.h"
#include "IRCHandler.h"

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

    char *buf = buffer.getDataPtr();
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
