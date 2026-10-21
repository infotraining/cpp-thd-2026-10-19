#ifndef CONCURRENT_QUEUE_HPP
#define CONCURRENT_QUEUE_HPP

#include <condition_variable>
#include <expected>
#include <mutex>
#include <optional>
#include <queue>

namespace ThreadLab
{
    template <class Q>
    concept BasicConcurrentQueue = std::move_constructible<typename Q::value_type> && requires(Q q, const Q cq, typename Q::value_type&& t) {
        { cq.is_closed() } noexcept -> std::same_as<bool>;
        { q.close() } noexcept -> std::same_as<void>;
        { q.push(std::forward<typename Q::value_type>(t)) } -> std::same_as<bool>;
        { q.pop() } -> std::same_as<std::optional<typename Q::value_type>>;
    };

    enum class ConqueueStatus {
        Empty,
        Closed,
        Busy
    };

    template <typename T>
    class ConcurrentQueue
    {
    public:
        using value_type = T;

        ConcurrentQueue() = default;

        [[nodiscard]] bool is_closed() const noexcept
        {
            std::lock_guard lock(mtx_);
            return is_closed_;
        }

        void close() noexcept
        {
            std::lock_guard lock(mtx_);
            is_closed_ = true;
            cv_is_not_empty_.notify_all();
        }

        bool push(const T& value)
        {
            {
                std::lock_guard lock(mtx_);
                if (is_closed_)
                    return false;
                queue_.push(value);
            }
            cv_is_not_empty_.notify_one();
            return true;
        }

        bool push(T&& value)
        {
            {
                std::lock_guard lock(mtx_);
                if (is_closed_)
                    return false;
                queue_.push(std::move(value));
            }
            cv_is_not_empty_.notify_one();
            return true;
        }

        [[nodiscard]] std::optional<T> pop()
        {
            std::unique_lock lock(mtx_);

            cv_is_not_empty_.wait(lock, [this] { return !queue_.empty() || is_closed_; });

            if (is_closed_ & queue_.empty())
                return std::nullopt;

            T value = std::move(queue_.front());
            queue_.pop();
            return value;
        }

        [[nodiscard]] std::expected<T, ConqueueStatus> try_pop()
        {
            std::unique_lock lock(mtx_, std::try_to_lock);

            if (!lock.owns_lock())
                return std::unexpected(ConqueueStatus::Busy);

            if (is_closed_ && queue_.empty())
                return std::unexpected(ConqueueStatus::Closed);

            if (queue_.empty())
            {
                return std::unexpected(ConqueueStatus::Empty);
            }

            T value = std::move(queue_.front());
            queue_.pop();
            return value;
        }

    private:
        std::queue<T> queue_;
        mutable std::mutex mtx_;
        std::condition_variable cv_is_not_empty_;
        bool is_closed_{false};
    };

    static_assert(BasicConcurrentQueue<ConcurrentQueue<int>>);

} // namespace ThreadLab

#endif // CONCURRENT_QUEUE_HPP
