//
// Created by l2pic on 25.04.2021.
//

#ifndef CHATCONTROLLER_IRC2_IRCWORKER_H_
#define CHATCONTROLLER_IRC2_IRCWORKER_H_

#include <thread>
#include <mutex>
#include <vector>
#include <algorithm>

#include "../common/SysSignal.h"

class IRCSession;
class IRCSelector
{
  public:
    explicit IRCSelector(size_t id);
    ~IRCSelector();

    void addSession(const std::shared_ptr<IRCSession> &session);
    void removeSession(const std::shared_ptr<IRCSession> &session);

  private:
    void run();

    size_t id = 0;

    std::mutex mutex;
    std::vector<std::shared_ptr<IRCSession>> sessions;

    bool needSync = false;
    std::vector<std::shared_ptr<IRCSession>> threadSafeCopy;

    std::thread thread;
};

#endif //CHATCONTROLLER_IRC2_IRCWORKER_H_
