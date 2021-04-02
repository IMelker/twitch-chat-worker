//
// Created by imelker on 06.03.2021.
//

#ifndef CHATBOT_COMMON_THREADPOOL_H_
#define CHATBOT_COMMON_THREADPOOL_H_


#include <memory>
#include <functional>
#include <queue>
#include <vector>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>

class ThreadPool
{
  public:
    explicit ThreadPool(size_t count = std::max(2u, std::thread::hardware_concurrency()));
    ~ThreadPool();

    size_t size() const { return workers.size(); };

    template<class F, class... Args>
    decltype(auto) enqueue(F &&f, Args &&... args);
  private:
    std::vector<std::thread> workers; // need to keep track of threads so we can join them
    std::queue<std::packaged_task<void()> > tasks; // the task queue

    // synchronization
    std::mutex mutex;
    std::condition_variable condition;
    bool stop;
};

// add new work item to the pool
template<class F, class... Args>
decltype(auto) ThreadPool::enqueue(F &&f, Args &&... args) {
    using return_type = std::invoke_result_t<F, Args...>;

    auto task = std::packaged_task<return_type()>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> res = task.get_future();
    {
        std::lock_guard<std::mutex> lock(this->mutex);
        // don't allow enqueueing after stopping the pool
        if (this->stop) {
            res.get();
            return res;
        }
        this->tasks.emplace(std::move(task));
    }
    this->condition.notify_one();
    return res;
}

#endif //CHATBOT_COMMON_THREADPOOL_H_
