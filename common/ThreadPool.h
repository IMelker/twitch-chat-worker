//
// Created by l2pic on 06.03.2021.
//

#ifndef CHATBOT_COMMON_THREADPOOL_H_
#define CHATBOT_COMMON_THREADPOOL_H_

#include <memory>
#include <vector>
#include <queue>
#include <future>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <stdexcept>

class ThreadPool
{
  public:
    explicit ThreadPool(size_t count);
    ~ThreadPool();

    template<class F, class... Args>
    auto enqueue(F &&f, Args &&... args) -> std::future<typename std::result_of<F(Args...)>::type>;
  private:
    std::vector<std::thread> workers; // need to keep track of threads so we can join them
    std::queue<std::function<void()> > tasks; // the task queue

    // synchronization
    std::mutex mutex;
    std::condition_variable condition;
    bool stop;
};

// add new work item to the pool
template<class F, class... Args>
auto ThreadPool::enqueue(F &&f, Args &&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared<std::packaged_task<return_type()> >(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(this->mutex);

        // don't allow enqueueing after stopping the pool
        if (this->stop) {
            //fprintf(stderr, "Pool stopped executing inplace\n");
            (*task)();
            return res;
        }

        this->tasks.emplace([task]() { (*task)(); });
    }
    this->condition.notify_one();
    return res;
}

#endif //CHATBOT_COMMON_THREADPOOL_H_
