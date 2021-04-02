#ifndef _IRCCLIENT_H
#define _IRCCLIENT_H

#include <string>
#include <utility>
#include <vector>
#include <functional>
#include <list>
#include <map>
#include <fstream>
#include <mutex>

#include "absl/strings/str_split.h"

#include "../common/BufferStatic.h"
#include "../common/Utils.h"

#include "IRCMessage.h"
#include "IRCSocket.h"

#define BUFF_SIZE (1024 * 4)

class Logger;
class IRCClient
{
  public:
    using IRCCommandHook = std::function<void(const IRCMessage&)>;
public:
    explicit IRCClient(std::shared_ptr<Logger>  logger);
    ~IRCClient() = default;

    bool connect(const char *host, int port);
    bool connected() { return this->ircSocket.connected(); };
    void disconnect();

    bool sendIRC(const std::string& data);

    bool login(const std::string& nick, const std::string& user, const std::string& password = std::string());

    void receive();
    void process(const char* data, size_t len);

    // Default internal handlers
    void handleCTCP(const IRCMessage& message);
    void handlePrivMsg(const IRCMessage& message);
    void handleNotice(const IRCMessage& message);
    void handleChannelJoinPart(const IRCMessage& message);
    void handleUserNickChange(const IRCMessage& message);
    void handleUserQuit(const IRCMessage& message);
    void handleChannelNamesList(const IRCMessage& message);
    void handleNicknameInUse(const IRCMessage& message);
    void handleServerMessage(const IRCMessage& message);

    void registerHook(std::string command, IRCCommandHook hook);
private:
    void callHook(const std::string& command, const IRCMessage& message);

    std::shared_ptr<Logger> logger;

    std::string nick;
    std::string user;

    BufferStatic<BUFF_SIZE> buffer;

    std::recursive_mutex mutex;
    IRCSocket ircSocket;

    std::multimap<std::string, IRCCommandHook, std::less<>> hooks;
};

#endif
