//
// Created by l2pic on 25.04.2021.
//

#ifndef CHATCONTROLLER_IRC2_IRCWORKERPOOL_H_
#define CHATCONTROLLER_IRC2_IRCWORKERPOOL_H_

#include <vector>
#include <mutex>
#include <memory>

class IRCSession;
class IRCSelector;
class IRCSelectorPool
{
  public:
    IRCSelectorPool();
    ~IRCSelectorPool();

    void init(size_t threads);

    void addSession(const std::shared_ptr<IRCSession> &session);
    void removeSession(const std::shared_ptr<IRCSession> &session);

  private:
    size_t curPos = 0;

    std::mutex mutex;
    std::vector<std::unique_ptr<IRCSelector>> workers;
};

#endif //CHATCONTROLLER_IRC2_IRCWORKERPOOL_H_
