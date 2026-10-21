#include "concurrent_queue.hpp"

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

using namespace std;
using namespace std::literals;
using namespace ThreadLab;

TEST_CASE("ConcurrentQueue - closing the queue")
{
    ConcurrentQueue<int> cq;

    SECTION("is open after creation")
    {
        REQUIRE(cq.is_closed() == false);
    }

    SECTION("can be closed")
    {
        cq.close();
        REQUIRE(cq.is_closed() == true);
    }

    SECTION("threads that waits on empty queue are unblocked when queue is closed")
    {
        std::latch ready_for_pop{1};
        std::atomic<bool> was_unblocked{false};

        auto pop_worker = std::thread([&] {
            ready_for_pop.count_down();
            std::optional<int> item = cq.pop();
            was_unblocked = true;
            REQUIRE(item == std::nullopt);
        });

        ready_for_pop.wait();
        std::this_thread::sleep_for(100ms);
        cq.close();

        pop_worker.join();
        REQUIRE(was_unblocked == true);
    }
}

TEST_CASE("ConcurrentQueue - basic push and pop operations")
{
    ConcurrentQueue<int> cq;

    SECTION("push on opened queue returns true")
    {
        CHECK(cq.is_closed() == false);

        REQUIRE(cq.push(1) == true);
    }

    SECTION("push on closed queue returns false")
    {
        cq.close();
        CHECK(cq.is_closed() == true);
        REQUIRE(cq.push(1) == false);
    }

    SECTION("push and pop - FIFO order")
    {
        cq.push(1);
        cq.push(2);
        cq.push(3);

        std::optional<int> item1 = cq.pop();
        std::optional<int> item2 = cq.pop();
        std::optional<int> item3 = cq.pop();

        REQUIRE(item1.has_value());
        REQUIRE(item1.value() == 1);
        REQUIRE(item2.has_value());
        REQUIRE(item2.value() == 2);
        REQUIRE(item3.has_value());
        REQUIRE(item3.value() == 3);
    }

    SECTION("pop on closed and empty queue returns std::nullopt")
    {
        cq.close();
        std::optional<int> item = cq.pop();
        REQUIRE(item == std::nullopt);
    }

    SECTION("pop on closed but non-empty queue returns item")
    {
        cq.push(1);
        cq.close();
        std::optional<int> item = cq.pop();
        REQUIRE(item.has_value());
        REQUIRE(item.value() == 1);
    }

    SECTION("pop on empty queue is blocking - waits for item to be pushed")
    {
        std::chrono::time_point<std::chrono::high_resolution_clock> pop_timepoint;

        std::latch ready_for_pop{1};

        auto pop_worker = std::thread([&] {
            ready_for_pop.count_down();
            std::optional<int> item = cq.pop();
            pop_timepoint = std::chrono::high_resolution_clock::now();
            REQUIRE(item.value() == 1);
        });

        ready_for_pop.wait();
        std::this_thread::sleep_for(100ms);
        std::chrono::time_point<std::chrono::high_resolution_clock> push_timepoint = std::chrono::high_resolution_clock::now();
        cq.push(1);

        pop_worker.join();
        REQUIRE(push_timepoint <= pop_timepoint);
    }
}

TEST_CASE("ConcurrentQueue - try_pop on empty queue")
{
    ConcurrentQueue<int> cq;

    SECTION("try_pop returns false when queue is empty")
    {
        int item;
        std::expected<int, ConqueueStatus> result = cq.try_pop();

        REQUIRE(result.has_value() == false);
        REQUIRE(result.error() == ConqueueStatus::Empty);
    }

    SECTION("try_pop returns false when queue is closed")
    {
        cq.close();
        std::expected<int, ConqueueStatus> result = cq.try_pop();
        REQUIRE(result.has_value() == false);
        REQUIRE(result.error() == ConqueueStatus::Closed);
    }

    SECTION("try_pop returns value when queue has items")
    {
        cq.push(1);
        cq.push(2);

        std::expected<int, ConqueueStatus> result = cq.try_pop();
        REQUIRE(result.has_value() == true);
        REQUIRE(result.value() == 1);

        result = cq.try_pop();
        REQUIRE(result.has_value() == true);
        REQUIRE(result.value() == 2);
    }
}

TEST_CASE("ConcurrentQueue - concurrent producers and consumers")
{
    constexpr int producer_count = 4;
    constexpr int consumer_count = 4;
    constexpr int items_per_producer = 5000;
    constexpr int total_items = producer_count * items_per_producer;

    ConcurrentQueue<int> cq;
    std::vector<int> consumed_items;
    consumed_items.reserve(total_items);
    std::mutex consumed_items_mutex;
    std::atomic<bool> all_pushes_succeeded{true};
    std::vector<std::thread> consumers;
    consumers.reserve(consumer_count);

    for (int i = 0; i < consumer_count; ++i)
    {
        consumers.emplace_back([&] {
            while (std::optional<int> item = cq.pop())
            {
                std::lock_guard lock(consumed_items_mutex);
                consumed_items.push_back(*item);
            }
        });
    }

    std::vector<std::thread> producers;
    producers.reserve(producer_count);
    for (int producer = 0; producer < producer_count; ++producer)
    {
        producers.emplace_back([&, producer] {
            for (int item = 0; item < items_per_producer; ++item)
            {
                if (!cq.push(producer * items_per_producer + item))
                {
                    all_pushes_succeeded = false;
                }
            }
        });
    }

    for (auto& producer : producers)
    {
        producer.join();
    }
    cq.close();

    for (auto& consumer : consumers)
    {
        consumer.join();
    }

    REQUIRE(all_pushes_succeeded);
    REQUIRE(consumed_items.size() == total_items);
    std::sort(consumed_items.begin(), consumed_items.end());
    for (int item = 0; item < total_items; ++item)
    {
        CHECK(consumed_items[item] == item);
    }
}

struct MoveOnly
{
    int value;

    MoveOnly() = default;
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;
    MoveOnly(MoveOnly&&) = default;
    MoveOnly& operator=(MoveOnly&&) = default;

    MoveOnly(int value)
        : value(value)
    { }

    bool operator==(const MoveOnly& other) const = default;
};

TEST_CASE("ConcurrentQueue - move-only types")
{
    SECTION("push and pop")
    {
        ConcurrentQueue<MoveOnly> cq_move;
        MoveOnly item1(1);
        MoveOnly item2(2);

        REQUIRE(cq_move.push(std::move(item1)) == true);
        REQUIRE(cq_move.push(std::move(item2)) == true);

        std::optional<MoveOnly> popped_item1 = cq_move.pop();
        std::optional<MoveOnly> popped_item2 = cq_move.pop();

        REQUIRE(popped_item1.has_value());
        REQUIRE(popped_item1->value == 1);
        REQUIRE(popped_item2.has_value());
        REQUIRE(popped_item2->value == 2);
    }

    SECTION("try_pop")
    {
        ConcurrentQueue<MoveOnly> cq;
        MoveOnly item1(1);
        REQUIRE(cq.push(std::move(item1)) == true);

        std::expected<MoveOnly, ConqueueStatus> result = cq.try_pop();

        REQUIRE(result.has_value());
        REQUIRE(result->value == 1);

        result = cq.try_pop();

        REQUIRE(result.has_value() == false);
        REQUIRE(result.error() == ConqueueStatus::Empty);
    }
}