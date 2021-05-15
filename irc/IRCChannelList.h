//
// Created by l2pic on 04.05.2021.
//

#ifndef CHATCONTROLLER__IRCCHANNELLIST_H_
#define CHATCONTROLLER__IRCCHANNELLIST_H_

#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <optional>
#include <map>

#include "IRCClientConfig.h"

class Logger;
class DBController;
class IRCSession;
class Channel {
  public:
    explicit Channel(std::string name) : name(std::move(name)) {};
    ~Channel() = default;

    Channel(Channel && other) = default;

    void attach(IRCSession *irc) { session = irc; }
    void detach() { session = nullptr; }

    [[nodiscard]] bool attachedTo(IRCSession *irc) const { return session == irc; }
    [[nodiscard]] bool attached() const { return session != nullptr; }

    [[nodiscard]] const std::string& getName() const { return name; }
    [[nodiscard]] IRCSession * getSession() const { return session; }
  private:
    std::string name;
    IRCSession *session = nullptr;
};

class IRCChannelList
{
  public:
    using Channels = std::map<std::string, Channel>;
    using Sessions = std::vector<std::shared_ptr<IRCSession>>;
    using ChannelsToSessionId = std::vector<std::pair<std::string, unsigned int>>;

  public:
    IRCChannelList(const Sessions& sessions, const IRCClientConfig& cliConfig, std::shared_ptr<Logger> logger,  std::shared_ptr<DBController> db);
    ~IRCChannelList();

    void load();

    bool inList(const std::string& name);
    void addChannel(Channel channel);
    std::optional<Channel> extractChannel(const std::string& name);
    void removeChannel(const std::string& name);

    void detachFromSession(IRCSession *session);
    std::vector<std::string> attachToSession(IRCSession *session);

    ChannelsToSessionId dumpChannelsToSessionId();

  private:
    const Sessions & sessions;
    const IRCClientConfig& cliConfig;

    const std::shared_ptr<Logger> logger;
    const std::shared_ptr<DBController> db;

    std::mutex mutex;
    Channels channels;
};

#endif //CHATCONTROLLER__IRCCHANNELLIST_H_
