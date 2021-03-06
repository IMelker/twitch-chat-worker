#ifndef _IRCCLIENT_H
#define _IRCCLIENT_H

#include <string>
#include <utility>
#include <vector>
#include <list>

#include <map>
#include <fstream>

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
            tokens = split(this->prefix, '@');
            this->nickname = tokens.at(0);
            this->host = tokens.at(1);
        }
        if (!this->nickname.empty() && this->nickname.find('!') != std::string::npos) {
            tokens = split(this->nickname, '!');
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

struct IRCCommandHook
{
    IRCCommandHook() : function(nullptr) {};

    std::string command;
    void (*function)(IRCMessage message, IRCClient* client);
};

class IRCClient
{
public:
    explicit IRCClient(bool debug = false) : debug(debug) {};
    ~IRCClient() {
        for(auto &[_,stream]: this->outstreams) {
            stream << "]" << std::endl;
        }
    }

    bool connect(char* host, int port);
    void disconnect();
    bool connected() { return this->ircsocket.connected(); };

    bool sendIRC(std::string data);

    bool login(const std::string& nick, const std::string& user, const std::string& password = std::string());

    void receive();

    void hookIRCCommand(std::string command, void (*function)(IRCMessage message, IRCClient* client));

    void parse(std::string data);

    void handleCTCP(IRCMessage message);

    // Default internal handlers
    void handlePrivMsg(IRCMessage message);
    void handleNotice(IRCMessage message);
    void handleChannelJoinPart(IRCMessage message);
    void handleUserNickChange(IRCMessage message);
    void handleUserQuit(IRCMessage message);
    void handleChannelNamesList(IRCMessage message);
    void handleNicknameInUse(IRCMessage message);
    void handleServerMessage(IRCMessage message);

    void setDebug(bool debug) { this->debug = debug; };

private:
    void handleCommand(const IRCMessage& message);
    void callHook(const std::string& command, const IRCMessage& message);

    IRCSocket ircsocket;

    std::list<IRCCommandHook> hooks;

    std::string nick;
    std::string user;

    std::map<std::string, std::ofstream> outstreams;

    bool debug;
};

#endif
