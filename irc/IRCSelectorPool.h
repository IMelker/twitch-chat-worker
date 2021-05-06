//
// Created by l2pic on 25.04.2021.
//

#ifndef CHATCONTROLLER_IRC_IRCWORKERPOOL_H_
#define CHATCONTROLLER_IRC_IRCWORKERPOOL_H_

#include <vector>
#include <mutex>
#include <memory>

class Logger;
class IRCSession;
class IRCSelector;
class IRCSelectorPool
{
  public:
    explicit IRCSelectorPool(std::shared_ptr<Logger> logger);
    ~IRCSelectorPool();

    void init(size_t threads);

    void addSession(const std::shared_ptr<IRCSession> &session);
    void removeSession(const std::shared_ptr<IRCSession> &session);

  private:
    IRCSelector *getNextSelectorRoundRobin();

    std::shared_ptr<Logger> logger;

    std::mutex mutex;
    size_t curSelectorRoundRobin = 0;
    std::vector<std::unique_ptr<IRCSelector>> selectors;
};

#endif //CHATCONTROLLER_IRC_IRCWORKERPOOL_H_
