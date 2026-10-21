#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <algorithm>
#include <cassert>
#include <concepts>
#include <functional>
#include <future>
#include <optional>
#include <thread>
#include <type_traits>
#include <vector>

#include "concurrent_queue.hpp"

namespace ThreadLab
{
    template <class T>
    concept BasicThreadPool = requires(T& pool) {
        { pool.submit([] { return 42; }) } -> std::same_as<std::future<int>>;
    };

    class ThreadPool
    {
        using Task = std::move_only_function<void()>;
    public:
        ThreadPool(size_t num_threads = 0)
        {
            if (num_threads == 0)
            {
                num_threads = std::max(1u, std::thread::hardware_concurrency());
            }

            assert(num_threads > 0);

            for(size_t i = 0; i < num_threads; ++i)
            {
                threads_.emplace_back([this] { run(); });
            }
        }

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        ~ThreadPool()
        {
            tasks_.close();

            for (auto& thd : threads_)
            {
                if (thd.joinable())
                {
                    thd.join();
                }
            }
        }

        template <typename F>
        auto submit(F&& f) -> std::future<std::invoke_result_t<std::decay_t<F>>>
        {
            using ResultType = std::invoke_result_t<std::decay_t<F>>;
            std::packaged_task<ResultType()> task(std::forward<F>(f));
            auto result = task.get_future();
            
            tasks_.push(Task(std::move(task)));
            
            return result;
        }

    private:
        ConcurrentQueue<Task> tasks_;
        std::vector<std::jthread> threads_;
        
        void run()
        {
            while (true)
            {
                std::optional<Task> task = tasks_.pop();
                if (task)
                {
                    (*task)();
                }
                else
                {
                    break;
                }
            }
        }

    };

} // namespace ThreadLab

#endif // THREAD_POOL_HPP
