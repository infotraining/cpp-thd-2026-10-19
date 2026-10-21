#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include "thread_safe_queue.hpp"

#include <cassert>
#include <functional>
#include <future>
#include <memory>
#include <thread>

#if __cplusplus >= 202302L
using Task = std::move_only_function<void()>;
#else
using Task = std::function<void()>;
#endif

class ThreadPool
{
    ThreadSafeQueue<Task> tasks_;
    std::vector<std::jthread> threads_;

    std::atomic<bool> is_done_{false};

public:
    ThreadPool(size_t no_of_threads = std::thread::hardware_concurrency())
    {
        threads_.reserve(no_of_threads);
        for (size_t i = 0; i < no_of_threads; ++i)
        {
            threads_.emplace_back(([this] { run(); }));
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ~ThreadPool()
    {
        for (size_t i; i < threads_.size(); ++i)
        {
            tasks_.push([this] { is_done_ = true; });
        }
    }

    template <typename TTask>
    auto submit(TTask&& task) -> std::future<decltype(task())>
    {
        using TResult = decltype(task());

#if __cplusplus >= 202302L
        std::packaged_task<TResult()> pt(std::forward<TTask>(task));
        std::future<TResult> f_result = pt.get_future();
        tasks_.push(std::move(pt));
#else
        // work-around for std::function as Task
        auto pt = std::make_shared<std::packaged_task<TResult()>>(std::forward<TTask>(task));
        std::future<TResult> f_result = pt->get_future();
        tasks_.push([pt] { (*pt)(); });
#endif

        return f_result;
    }

private:
    void run()
    {
        Task task;

        while (true)
        {
            tasks_.pop(task);
            task(); // running task in this thread

            if (is_done_)
            {
                return;
            }
        }
    }
};

#endif // THREAD_POOL_HPP
