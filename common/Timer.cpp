//
// Created by l2pic on 08.04.2021.
//

#include <chrono>

#include "SysSignal.h"
#include "Timer.h"
#include "Clock.h"

#define TIMER_STEP 100

Timer::Timer() : active(true) {
    thread = std::thread(&Timer::run, this);
}

Timer::~Timer() {
    if (thread.joinable())
        thread.join();
}

void Timer::setTimer(Timer::Function function, long long timestamp) {
    std::lock_guard lg(mutex);
    queue.emplace(timestamp, std::make_pair(std::move(function), 0));
}

void Timer::setTimeout(Function function, long long delayMs) {
    long long timestamp = CurrentTime<std::chrono::system_clock>::milliseconds() + delayMs;
    setTimer(std::move(function), timestamp);
}

void Timer::setInterval(Function function, long long intervalMs) {
    long long timestamp = CurrentTime<std::chrono::system_clock>::milliseconds() + intervalMs;

    std::lock_guard lg(mutex);
    queue.emplace(timestamp, std::make_pair(std::move(function), intervalMs));
}

void Timer::stop() {
    active.store(false, std::memory_order_relaxed);
}

void Timer::run() {
    while (!SysSignal::serviceTerminated() && active.load(std::memory_order_relaxed)) {
        auto now = CurrentTime<std::chrono::system_clock>::milliseconds();
        {
            std::lock_guard lg(mutex);
            auto it = queue.begin();
            while (it != queue.end()) {
                if (it->first > now)
                    break;

                it->second.first();

                int repeat = it->second.second;
                if (repeat > 0) {
                    auto next = std::next(it);

                    auto nh = queue.extract(it);
                    nh.key() = now + repeat;
                    queue.insert(std::move(nh));

                    it = next;
                } else {
                    it = queue.erase(it);
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIMER_STEP));
    }
}

