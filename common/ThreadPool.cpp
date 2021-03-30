//
// Created by l2pic on 06.03.2021.
//

#include "ThreadPool.h"

// the constructor just launches some amount of workers
ThreadPool::ThreadPool(size_t threads) : stop(false) {
    for (size_t i = 0; i < threads; ++i) {
        this->workers.emplace_back([this] () {
            for (;;) {
                std::packaged_task<void()> task;

                {
                    std::unique_lock<std::mutex> lock(this->mutex);
                    this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                    if (this->stop && this->tasks.empty())
                        return;
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }

                try {
                    task();
                } catch (...) {
                    abort();
                }
            }
        });
    }
}

// the destructor joins all threads
ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(this->mutex);
        this->stop = true;
    }
    this->condition.notify_all();
    for (std::thread &worker: this->workers)
        worker.join();
}

