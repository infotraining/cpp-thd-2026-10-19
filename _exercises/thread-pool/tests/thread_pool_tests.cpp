#include "thread_pool.hpp"
#include "thread_safe_queue.hpp"

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <queue>
#include <thread>
#include <iterator>

using namespace std;

TEST_CASE("ThreadPool")
{
    SECTION("executes task in a pool of threads")
    {
        std::atomic<bool> is_completed = false;

        {
            ThreadPool thd_pool{8};
            thd_pool.submit([&] { is_completed = true; });
        }

        REQUIRE(is_completed == true);
    }
}

TEST_CASE("STL")
{
    std::vector<int> vec;

    int i = 0;
    std::generate_n(std::back_inserter(vec), 10, [i]() mutable { return ++i; });

    for(const auto& item : vec)
    {
        std::cout << item << " ";
    }
    std::cout << "\n";

    std::vector<int> evens;
    evens.reserve(vec.size());
    std::copy_if(vec.begin(), vec.end(), std::back_inserter(evens), 
                 [](int n) { return n % 2 == 0; });

    // for(const auto& item : vec)
    // {
    //     if (item % 2 == 0)
    //         evens.push_back(item);
    // }

    std::copy(evens.begin(), evens.end(), std::ostream_iterator<int>(std::cout, " "));

    // for(const auto& item : evens)
    // {
    //     std::cout << item << " ";
    // }
    std::cout << "\n";
}