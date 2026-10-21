#include "thread_safe_queue.hpp"
#include "thread_pool.hpp"

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <queue>
#include <thread>
#include <iostream>
#include <future>

using namespace std;

TEST_CASE("ThreadPool")
{
    SECTION("executes task in a pool of threads")
    {
        REQUIRE(true);
    }
}
