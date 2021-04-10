//
// Created by l2pic on 08.04.2021.
//

#ifndef CHATCONTROLLER_COMMON_TIMER_H_
#define CHATCONTROLLER_COMMON_TIMER_H_

#include <thread>
#include <atomic>
#include <mutex>
#include <map>
#include <functional>

class Timer
{
    using Function = std::function<void()>;
  public:
    static Timer &instance() {
        static Timer timer;
        return timer;
    }

    Timer(const Timer &) = delete;
    Timer &operator=(const Timer &) = delete;

    void setTimer(Function function, long long timestamp);
    void setTimeout(Function function, long long delayMs);
    void setInterval(Function function, long long intervalMs);

    void stop();
  private:
    Timer();
    ~Timer();

    void run();

    std::atomic_bool active;
    std::thread thread;

    std::mutex mutex;
    std::multimap<long long, std::pair<Function, int>> queue;
};

#endif //CHATCONTROLLER_COMMON_TIMER_H_
