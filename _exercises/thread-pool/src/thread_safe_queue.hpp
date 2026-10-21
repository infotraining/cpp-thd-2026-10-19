#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP

#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T>
class ThreadSafeQueue
{
    std::queue<T> queue_;
    mutable std::mutex mtx_queue_;
    std::condition_variable cv_q_not_empty_;

public:
    ThreadSafeQueue() = default;

    bool empty() const
    {
        std::lock_guard<std::mutex> lk(mtx_queue_);
        return queue_.empty();
    }

    void push(const T& value)
    {
        {
            std::lock_guard<std::mutex> lk(mtx_queue_);
            queue_.push(value);
        }
        cv_q_not_empty_.notify_one();
    }

    void push(T&& value)
    {
        {
            std::lock_guard<std::mutex> lk(mtx_queue_);
            queue_.push(std::move(value));
        }
        cv_q_not_empty_.notify_one();
    }

    void push(std::initializer_list<T> items)
    {
        {
            std::lock_guard<std::mutex> lock(mtx_queue_);
            for (const T& item : items)
                queue_.push(item);
        }
        cv_q_not_empty_.notify_all();
    }

    void pop(T& item)
    {
        std::unique_lock<std::mutex> lock(mtx_queue_);
        cv_q_not_empty_.wait(lock, [this] { return !queue_.empty(); });

        item = std::move(queue_.front());
        queue_.pop();
    }

    bool try_pop(T& item)
    {
        std::unique_lock lock(mtx_queue_, std::try_to_lock);
        if (!lock.owns_lock() || queue_.empty())
        {
            return false;
        }

        item = std::move(queue_.front());
        queue_.pop();

        return true;
    }
};

#endif // THREAD_SAFE_QUEUE_HPP
