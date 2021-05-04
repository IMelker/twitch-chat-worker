//
// Created by l2pic on 25.04.2021.
//

#include "IRCSelector.h"
#include "IRCSelectorPool.h"

IRCSelectorPool::IRCSelectorPool() = default;
IRCSelectorPool::~IRCSelectorPool() = default;

void IRCSelectorPool::init(size_t threads) {
    std::lock_guard lg(mutex);
    selectors.reserve(threads);
    for (size_t i = 0; i < threads; ++i) {
        selectors.emplace_back(new IRCSelector(i));
    }
}

void IRCSelectorPool::addSession(const std::shared_ptr<IRCSession> &session) {
    std::lock_guard lg(mutex);
    getNextSelectorRoundRobin()->addSession(session);
}

void IRCSelectorPool::removeSession(const std::shared_ptr<IRCSession> &session) {
    std::lock_guard lg(mutex);
    for(auto &selector: selectors) {
        selector->removeSession(session);
    }
}

IRCSelector *IRCSelectorPool::getNextSelectorRoundRobin() {
    if (selectors.size() == 1)
        return selectors.front().get();

    unsigned int i = curSelectorRoundRobin++;
    curSelectorRoundRobin %= selectors.size();
    return selectors[i].get();
}
