#ifndef _IRCCLIENT_H
#define _IRCCLIENT_H

#include <string>
#include <utility>
#include <vector>
#include <functional>
#include <list>
#include <map>
#include <fstream>

#include "absl/strings/str_split.h"

#include "../common/Utils.h"

#include "IRCSocket.h"

class IRCClient;

struct IRCCommandPrefix
{
    void parse(const std::string &data) {
        if (data.empty())
            return;

        this->prefix = data.substr(1, data.find(' ') - 1);

        std::vector<std::string> tokens;
        if (this->prefix.find('@') != std::string::npos) {
            tokens = absl::StrSplit(this->prefix, '@');
            this->nickname = tokens.at(0);
            this->host = tokens.at(1);
        }
        if (!this->nickname.empty() && this->nickname.find('!') != std::string::npos) {
            tokens = absl::StrSplit(this->nickname, '!');
            this->nickname = tokens.at(0);
            this->username = tokens.at(1);
        }
    };

    std::string prefix;
    std::string nickname;
    std::string username;
    std::string host;
};

class IRCMessage
{
  public:
    IRCMessage(std::string cmd, IRCCommandPrefix p, std::vector<std::string> params) :
        command(std::move(cmd)), prefix(std::move(p)), parameters(std::move(params)) {};

    std::string command;
    IRCCommandPrefix prefix;
    std::vector<std::string> parameters;
};


class IRCClient
{
  public:
    using IRCCommandHook = std::function<void(IRCMessage)>;
public:
    explicit IRCClient() = default;
    ~IRCClient() = default;

    bool connect(const char *host, int port);
    bool connected() { return this->ircsocket.connected(); };
    void disconnect();

    bool sendIRC(const std::string& data);

    bool login(const std::string& nick, const std::string& user, const std::string& password = std::string());

    void receive();
    void parse(std::string data);

    // Default internal handlers
    void handleCTCP(IRCMessage message);
    void handlePrivMsg(IRCMessage message);
    void handleNotice(IRCMessage message);
    void handleChannelJoinPart(IRCMessage message);
    void handleUserNickChange(IRCMessage message);
    void handleUserQuit(IRCMessage message);
    void handleChannelNamesList(IRCMessage message);
    void handleNicknameInUse(IRCMessage message);
    void handleServerMessage(IRCMessage message);

    void registerHook(std::string command, IRCCommandHook hook);
private:
    void callHook(const std::string& command, const IRCMessage& message);

    std::string nick;
    std::string user;
    IRCSocket ircsocket;

    std::multimap<std::string, IRCCommandHook> hooks;
};

#endif
