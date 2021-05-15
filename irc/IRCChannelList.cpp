//
// Created by l2pic on 04.05.2021.
//

#include "Logger.h"
#include "../DBController.h"
#include "IRCChannelList.h"
#include "IRCSession.h"

inline void fillChannelsMap(const std::shared_ptr<DBController> db, std::map<std::string, Channel>& target) {
    auto list = db->loadChannels();
    std::transform(list.begin(), list.end(), std::inserter(target, target.end()), [] (auto& name) {
        return std::make_pair(name, Channel{name});
    });
}

IRCChannelList::IRCChannelList(const Sessions &sessions, const IRCClientConfig& cliConfig, std::shared_ptr<Logger> logger, std::shared_ptr<DBController> db)
  : sessions(sessions), cliConfig(cliConfig), logger(std::move(logger)), db(std::move(db)) {

}


IRCChannelList::~IRCChannelList() = default;

void IRCChannelList::load() {
    logger->logTrace("IRCChannelList[{}] Load channels inited for account id={}",
                     fmt::ptr(this), cliConfig.id);
    auto loadedChannels = db->loadChannelsFor(cliConfig.id);

    Channels temp;
    std::transform(loadedChannels.begin(), loadedChannels.end(),
                   std::inserter(temp, temp.end()), [] (auto& name) {
        return std::make_pair(name, Channel{name});
    });

    logger->logInfo("IRCChannelList[{}] loaded {} channels, that will be joined to {} sessions",
                     fmt::ptr(this), loadedChannels.size(), cliConfig.session_count);

    std::lock_guard lg(mutex);
    channels.swap(temp);
}

void IRCChannelList::addChannel(Channel channel) {
    std::lock_guard lg(mutex);
    channels.emplace(channel.getName(), std::move(channel));
}

std::optional<Channel> IRCChannelList::extractChannel(const std::string &name) {
    std::lock_guard lg(mutex);
    auto it = channels.find(name);
    if (it == channels.end())
        return {};

    Channel res{std::move(it->second)};
    channels.erase(it);
    return res;
}

void IRCChannelList::removeChannel(const std::string &name) {
    std::lock_guard lg(mutex);
    channels.erase(name);
}

std::vector<std::string> IRCChannelList::attachToSession(IRCSession *session) {
    std::vector<std::string> result;

    std::lock_guard lg(mutex);
    size_t capacity = (channels.size() / cliConfig.session_count) + 1;
    result.reserve(capacity);

    for (auto &[name, channel]: channels) {
        if (channel.attached())
            continue;

        channel.attach(session);
        result.push_back(channel.getName());

        if (result.size() == capacity)
            break;
    }

    return result;
}

void IRCChannelList::detachFromSession(IRCSession *session) {
    std::lock_guard lg(mutex);
    for (auto &[name, channel]: channels)
        if (channel.attachedTo(session))
            channel.detach();
}

bool IRCChannelList::inList(const std::string &name) {
    std::lock_guard lg(mutex);
    return channels.find(name) != channels.end();
}

IRCChannelList::ChannelsToSessionId IRCChannelList::dumpChannelsToSessionId() {
    ChannelsToSessionId res;
    {
        std::lock_guard lg(mutex);
        res.reserve(channels.size());
        std::transform(channels.begin(), channels.end(), std::back_inserter(res), [] (const auto& pair) {
            const auto &[name, channel] = pair;
            auto *session = channel.getSession();
            return std::make_pair(name, session ? session->getId() : -1/*max unsigned int*/);
        });
    }
    return res;
}
