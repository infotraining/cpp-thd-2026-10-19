#include "thread_pool.hpp"

#include <algorithm>
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <condition_variable>
#include <expected>
#include <functional>
#include <future>
#include <iostream>
#include <latch>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <array>

using namespace std;
using namespace std::literals;
using namespace ThreadLab;

// CPU-bound task; cost grows with `limit` (trial division)
int64_t count_primes(int64_t limit)
{
    int64_t count = 0;
    for (int64_t n = 2; n < limit; ++n)
    {
        bool is_prime = true;
        for (int64_t d = 2; d * d <= n; ++d)
        {
            if (n % d == 0)
            {
                is_prime = false;
                break;
            }
        }
        count += is_prime;
    }
    return count;
}

// varied per-task workload: 1'000 .. 5'000
int64_t work_limit(int s, int i)
{
    return 1'000 + (s * 31 + i * 17) % 4'000;
}

TEST_CASE("count_primes", "[thread_pool]")
{
    REQUIRE(count_primes(2) == 0);
    REQUIRE(count_primes(100) == 25);
    REQUIRE(count_primes(10'000) == 1229);
}

TEST_CASE("ThreadPool basic functionality", "[thread_pool]")
{
    SECTION("submit and shutdown")
    {
        std::atomic<int> counter{0};
        {
            ThreadPool pool{4};

            auto submit_tasks = [&pool, &counter](int num_tasks) {
                for (int i = 0; i < num_tasks; ++i)
                {
                    pool.submit([&counter] {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        ++counter;
                    });
                }
            };

            {
                std::vector<std::jthread> submitters;

                for (int i = 0; i < 10; ++i)
                {
                    submitters.emplace_back(submit_tasks, 25);
                }
            }
        }

        REQUIRE(counter == 250);
    }

    SECTION("submit returns future")
    {
        ThreadPool pool{2};

        auto future_result = pool.submit([] {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return 42;
        });

        REQUIRE(future_result.get() == 42);
    }

    SECTION("stress test with futures")
    {
        constexpr int num_submitters = 8;
        constexpr int tasks_per_submitter = 500;

        const std::array<std::array<int64_t, tasks_per_submitter>, num_submitters> expected = [] {
            std::array<std::array<int64_t, tasks_per_submitter>, num_submitters> arr{};
            for (int s = 0; s < num_submitters; ++s)
            {
                for (int i = 0; i < tasks_per_submitter; ++i)
                {
                    arr[s][i] = count_primes(work_limit(s, i));
                }
            }
            return arr;
        }();

        ThreadPool pool{4};
        std::atomic<int32_t> executed{0};
        std::vector<std::vector<std::future<int64_t>>> results(num_submitters);

        {
            std::vector<std::jthread> submitters;

            for (int s = 0; s < num_submitters; ++s)
            {
                submitters.emplace_back([&pool, &executed, &results, s] {
                    results[s].reserve(tasks_per_submitter);
                    for (int i = 0; i < tasks_per_submitter; ++i)
                    {
                        const int64_t limit = work_limit(s, i);
                        results[s].push_back(pool.submit([&executed, limit] {
                            ++executed;
                            return count_primes(limit);
                        }));
                    }
                });
            }
        }

        for (int s = 0; s < num_submitters; ++s)
        {
            for (int i = 0; i < tasks_per_submitter; ++i)
            {
                REQUIRE(results[s][i].get() == expected[s][i]);
            }
        }

        REQUIRE(executed == num_submitters * tasks_per_submitter);
    }
}
