#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <barrier>

#include "synchronized.hpp"

using namespace std::literals;

void test_synchronized_lock_in_scope()
{
    using ThreadLab::Synchronized;

    Synchronized<int> counter;

    {
        auto locked_value = counter.lock();
        *locked_value += 1;
        *locked_value += 2;
    }

    assert(*counter.lock() == 3);
}

void test_synchronized_lock_with_struct()
{
    using ThreadLab::Synchronized;

    Synchronized<std::vector<int>> vec;

    vec.lock()->push_back(0);

    {
        auto locked_value = vec.lock();
        locked_value->push_back(1);
        locked_value->push_back(2);
    }

    assert(*vec.lock() == (std::vector<int>{0, 1, 2}));
}

void test_synchronized_with_lock()
{
    using ThreadLab::Synchronized;

    Synchronized<int> counter;

    counter.with_lock([](int& value) {
        value += 1;
        value += 2;
    });

    assert(*counter.lock() == 3);
}

void test_synchronized_for_const_access()
{
    using ThreadLab::Synchronized;

    Synchronized<int> counter;

    auto increment_counter = [](Synchronized<int>& counter) {
        counter.with_lock([](int& value) {
            value += 1;
        });
    };

    auto read_counter = [](const Synchronized<int>& counter, Synchronized<int>& output) {
        auto locked_value = counter.lock();

        output.with_lock([&](int& value) {
            value = *locked_value;
        });
    };

    std::jthread thd_1{[&] { increment_counter(counter); }};
    std::jthread thd_2{[&] { increment_counter(counter); }};

    Synchronized<int> output_counter;

    std::jthread thd_3{[&] { read_counter(counter, output_counter); }};
    thd_1.join();
    thd_2.join();
    thd_3.join();

    assert(*output_counter.lock() == 2);
}

void test_synchronized_initial_value_and_const_access()
{
    using ThreadLab::Synchronized;

    const Synchronized<int> counter{42};

    assert(*counter.lock() == 42);
    assert(counter.with_lock([](const int& value) { return value; }) == 42);
}

void test_synchronized_with_lock_returns_value()
{
    using ThreadLab::Synchronized;

    Synchronized<int> counter{1};

    const int result = counter.with_lock([](int& value) {
        value += 2;
        return value;
    });

    assert(result == 3);
    assert(*counter.lock() == 3);
}

void test_synchronized_unlocks_after_exception()
{
    using ThreadLab::Synchronized;

    Synchronized<int> counter;
    bool exception_caught = false;

    try
    {
        counter.with_lock([](int&) {
            throw std::runtime_error("callback failed");
        });
    }
    catch (const std::runtime_error&)
    {
        exception_caught = true;
    }

    assert(exception_caught);

    counter.with_lock([](int& value) { ++value; });
    assert(*counter.lock() == 1);
}

void test_synchronized_concurrent_updates()
{
    using ThreadLab::Synchronized;

    constexpr int thread_count = 8;
    constexpr int increments_per_thread = 1000;
    Synchronized<int> counter;

    {
        std::vector<std::jthread> threads;
        for (int i = 0; i < thread_count; ++i)
        {
            threads.emplace_back([&counter] {
                for (int j = 0; j < increments_per_thread; ++j)
                {
                    counter.with_lock([](int& value) { ++value; });
                }
            });
        }
    }

    assert(*counter.lock() == thread_count * increments_per_thread);
}

void test_synchronized_stress()
{
    using ThreadLab::Synchronized;

    struct State
    {
        int first{};
        int second{};
    };

    constexpr int writer_count = 8;
    constexpr int reader_count = 2;
    constexpr int iterations = 100'000;
    Synchronized<State> state;
    const Synchronized<State>& read_only_state = state;
    std::barrier start_line{writer_count + reader_count + 1};
    std::atomic<bool> snapshots_consistent{true};
    std::vector<std::jthread> threads;
    threads.reserve(writer_count + reader_count);

    for (int i = 0; i < writer_count; ++i)
    {
        threads.emplace_back([&] {
            start_line.arrive_and_wait();
            for (int j = 0; j < iterations; ++j)
            {
                state.with_lock([](State& value) {
                    ++value.first;
                    ++value.second;
                });
            }
        });
    }

    for (int i = 0; i < reader_count; ++i)
    {
        threads.emplace_back([&] {
            start_line.arrive_and_wait();
            for (int j = 0; j < iterations; ++j)
            {
                const bool consistent =
                    read_only_state.with_lock([](const State& value) {
                        return value.first == value.second;
                    });
                if (!consistent)
                {
                    snapshots_consistent.store(false);
                }
            }
        });
    }

    start_line.arrive_and_wait();
    for (auto& thread : threads)
    {
        thread.join();
    }

    assert(snapshots_consistent.load());
    assert(read_only_state.with_lock([](const State& value) {
        return value.first == writer_count * iterations
            && value.second == writer_count * iterations;
    }));
}

int main()
{
    test_synchronized_lock_in_scope();
    test_synchronized_lock_with_struct();
    test_synchronized_with_lock();
    test_synchronized_for_const_access();
    test_synchronized_initial_value_and_const_access();
    test_synchronized_with_lock_returns_value();
    test_synchronized_unlocks_after_exception();
    test_synchronized_concurrent_updates();
    test_synchronized_stress();
}
