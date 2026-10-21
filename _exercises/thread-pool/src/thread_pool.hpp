#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include "thread_safe_queue.hpp"

#include <cassert>
#include <functional>
#include <thread>

class ThreadPool
{
    using Task = std::function<void()>;
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
        for(size_t i; i < threads_.size(); ++i)
        {
            tasks_.push([this] { is_done_ = true; });
        }
    }   

    void submit(Task task)
    {
        assert(task == true);

        tasks_.push(task);
    }

private:
    void run()
    {
        Task task;
        
        while(true)
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
