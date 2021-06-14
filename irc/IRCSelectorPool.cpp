//
// Created by l2pic on 25.04.2021.
//

#include "Logger.h"
#include "IRCSelector.h"
#include "IRCSelectorPool.h"

IRCSelectorPool::IRCSelectorPool(std::shared_ptr<Logger> logger)
  : logger(std::move(logger)) {

};

IRCSelectorPool::~IRCSelectorPool() = default;

void IRCSelectorPool::init(size_t threads) {
    std::lock_guard lg(mutex);
    selectors.reserve(threads);
    for (size_t i = 0; i < threads; ++i) {
        selectors.emplace_back(new IRCSelector(i, logger.get()));
    }
}

void IRCSelectorPool::addSession(const std::shared_ptr<IRCSession> &session) {
    std::shared_ptr<IRCSession> tempHolder = session;

    std::lock_guard lg(mutex);
    getNextSelectorRoundRobin()->addSession(tempHolder);
}

void IRCSelectorPool::removeSession(const std::shared_ptr<IRCSession> &session) {
    std::shared_ptr<IRCSession> tempHolder = session;

    std::lock_guard lg(mutex);
    for(auto &selector: selectors) {
        selector->removeSession(tempHolder);
    }
}

IRCSelector *IRCSelectorPool::getNextSelectorRoundRobin() {
    if (selectors.size() == 1)
        return selectors.front().get();

    unsigned int i = curSelectorRoundRobin++;
    curSelectorRoundRobin %= selectors.size();
    return selectors[i].get();
}
