//
// Created by l2pic on 25.04.2021.
//

#include "IRCSelector.h"
#include "IRCSelectorPool.h"

IRCSelectorPool::IRCSelectorPool() = default;
IRCSelectorPool::~IRCSelectorPool() = default;

void IRCSelectorPool::init(size_t threads) {
    std::lock_guard lg(mutex);
    workers.reserve(threads);
    for (size_t i = 0; i < threads; ++i) {
        workers.emplace_back(new IRCSelector(i));
    }
}

void IRCSelectorPool::addSession(const std::shared_ptr<IRCSession> &session) {
    std::lock_guard lg(mutex);
    workers[curPos++ % workers.size()]->addSession(session);
}

void IRCSelectorPool::removeSession(const std::shared_ptr<IRCSession> &session) {
    std::lock_guard lg(mutex);
    for(auto &worker: workers) {
        worker->removeSession(session);
    }
}
