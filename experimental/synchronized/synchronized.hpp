#ifndef SYNCHRONIZED_HPP
#define SYNCHRONIZED_HPP

namespace ThreadLab
{
    template <typename T>
    concept BasicLockable = requires(T& m) {
        m.lock();
        m.unlock();
    };

    template <typename T>
    concept Lockable = BasicLockable<T> && requires(T& m) {
        { m.try_lock() } -> std::same_as<bool>;
    };

    template <typename T, typename TMutex>
    class LockedPtr
    {
        T& ref_;
        TMutex& mutex_;

    public:
        LockedPtr(T& ref, TMutex& mutex)
            : ref_(ref)
            , mutex_(mutex)
        {
            mutex_.lock();
        }

        LockedPtr(const LockedPtr&) = delete;
        LockedPtr& operator=(const LockedPtr&) = delete;

        ~LockedPtr() noexcept
        {
            mutex_.unlock();
        }

        T* operator->() noexcept
            requires std::is_class_v<T>
        {
            return &ref_;
        }

        T& operator*() noexcept
        {
            return ref_;
        }
    };

    template <typename T, BasicLockable TMutex = std::mutex>
    class Synchronized
    {
        T value_{};
        mutable TMutex mutex_;

    public:
        Synchronized() = default;

        explicit Synchronized(T value) noexcept(std::is_nothrow_move_constructible_v<T>)
            : value_(std::move(value))
        { }

        Synchronized(const Synchronized&) = delete;
        Synchronized& operator=(const Synchronized&) = delete;

        Synchronized(Synchronized&&) = delete;
        Synchronized& operator=(Synchronized&&) = delete;

        LockedPtr<T, TMutex> lock()
        {
            return LockedPtr<T, TMutex>(value_, mutex_);
        }

        LockedPtr<const T, TMutex> lock() const
        {
            return LockedPtr<const T, TMutex>(value_, mutex_);
        }

        template <std::invocable<T&> Func>
        decltype(auto) with_lock(Func&& func)
        {
            auto locked = lock();
            return std::forward<Func>(func)(*locked);
        }

        template <std::invocable<T&> Func>
        decltype(auto) with_lock(Func&& func) const
        {
            auto locked = lock();
            return std::forward<Func>(func)(*locked);
        }

        TMutex& as_lockable() noexcept
        {
            return mutex_;
        }
    };
} // namespace ThreadLab

#endif // SYNCHRONIZED_SYNCHRONIZED_HPP