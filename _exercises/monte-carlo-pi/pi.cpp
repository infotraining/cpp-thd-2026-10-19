#include <atomic>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <numeric>
#include <random>
#include <ranges>
#include <thread>
#include <mutex>
#include <future>

const uintmax_t N = 1'000'000;

//---------------------------------------------------------------------------------------
// Function to calculate the number of hits inside the unit circle for Monte Carlo Pi estimation
// Hits counter passed by reference incremented in hot loop - may cause false sharing in multi-threaded scenarios
void calculate_hits(uintmax_t N, uintmax_t& hits)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    for (uintmax_t n = 0; n < N; ++n)
    {
        double x = dist(gen);
        double y = dist(gen);
        if (x * x + y * y <= 1)
            ++hits;
    }
}

double mc_pi_single_thread(uintmax_t N)
{
    uintmax_t hits = 0;
    calculate_hits(N, hits);

    return static_cast<double>(hits) / N * 4;
}

double mc_pi_multi_thread(uintmax_t N, uintmax_t num_threads = std::max(1u, std::thread::hardware_concurrency()))
{
    std::vector<uintmax_t> hits(num_threads, 0);
    uintmax_t chunk_size = N / num_threads;

    {
        std::vector<std::jthread> threads;
        for (uintmax_t i = 0; i < num_threads; ++i)
        {
            threads.emplace_back(calculate_hits, chunk_size, std::ref(hits[i]));
        }
    } // implicit join

    uintmax_t total_hits = std::accumulate(hits.begin(), hits.end(), 0ULL);

    return static_cast<double>(total_hits) / N * 4;
}

//---------------------------------------------------------------------------------------
// Function to calculate the number of hits inside the unit circle for Monte Carlo Pi estimation
// Uses a local counter to avoid false sharing in multi-threaded scenarios

void calculate_hits_with_local_counter(uintmax_t N, uintmax_t& hits)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    uintmax_t local_hits = 0;
    for (uintmax_t n = 0; n < N; ++n)
    {
        double x = dist(gen);
        double y = dist(gen);
        if (x * x + y * y <= 1)
            ++local_hits;
    }

    hits += local_hits;
}

double mc_pi_multi_thread_with_local_counter(uintmax_t N, uintmax_t num_threads = std::max(1u, std::thread::hardware_concurrency()))
{
    std::vector<uintmax_t> hits(num_threads, 0);
    uintmax_t chunk_size = N / num_threads;

    {
        std::vector<std::jthread> threads;
        for (uintmax_t i = 0; i < num_threads; ++i)
        {
            threads.emplace_back(calculate_hits_with_local_counter, chunk_size, std::ref(hits[i]));
        }
    } // implicit join

    uintmax_t total_hits = std::accumulate(hits.begin(), hits.end(), 0ULL);

    return static_cast<double>(total_hits) / N * 4;
}

//---------------------------------------------------------------------------------------
// Monte-Carlo with aligned counter
// Uses an aligned counter (to size of a cache line) to avoid false sharing in multi-threaded scenarios

struct Counter
{
    alignas(std::hardware_destructive_interference_size) uintmax_t value = 0;

    Counter& operator++()
    {
        ++value;
        return *this;
    }

    operator uintmax_t() const
    {
        return value;
    }
};

void calculate_hits_with_aligned_counter(uintmax_t N, Counter& hits)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    for (uintmax_t n = 0; n < N; ++n)
    {
        double x = dist(gen);
        double y = dist(gen);
        if (x * x + y * y <= 1)
            ++hits;
    }
}

double mc_pi_multi_thread_with_aligned_counter(uintmax_t N, uintmax_t num_threads = std::max(1u, std::thread::hardware_concurrency()))
{
    std::vector<Counter> hits(num_threads);
    uintmax_t chunk_size = N / num_threads;

    {
        std::vector<std::jthread> threads;
        for (uintmax_t i = 0; i < num_threads; ++i)
        {
            threads.emplace_back(calculate_hits_with_aligned_counter, chunk_size, std::ref(hits[i]));
        }
    } // implicit join

    uintmax_t total_hits = std::accumulate(hits.begin(), hits.end(), 0ULL);

    return static_cast<double>(total_hits) / N * 4;
}

//---------------------------------------------------------------------------------------
// Monte-Carlo Pi with Mutex

void calculate_hits_mutex_hot_loop(uintmax_t N, uintmax_t& hits, std::mutex& mtx)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    for (uintmax_t n = 0; n < N; ++n)
    {
        double x = dist(gen);
        double y = dist(gen);
        if (x * x + y * y <= 1)
        {
            std::lock_guard lk{mtx};
            ++hits; // hot loop
        }
    }
}

void calculate_hits_mutex_late_update(uintmax_t N, uintmax_t& hits, std::mutex& mtx)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    uintmax_t local_hits = 0;
    for (uintmax_t n = 0; n < N; ++n)
    {
        double x = dist(gen);
        double y = dist(gen);
        if (x * x + y * y <= 1)
        {
            ++local_hits; // hot loop
        }
    }

    // late update
    {
        std::lock_guard lk{mtx};
        hits += local_hits;
    }
}

double mc_pi_multi_thread_mutex(uintmax_t N)
{
    uintmax_t hits = 0;
    std::mutex mtx;

    int num_cores = std::thread::hardware_concurrency();

    std::vector<std::thread> threads;
    std::vector<uintmax_t> results(num_cores, 0);

    for (int i = 0; i < num_cores; ++i)
    {
        //std::cout << "Launching thread " << i << std::endl;
        threads.push_back(std::thread([&, i]{
            calculate_hits_mutex_hot_loop(N / num_cores, hits, mtx);
        }));
    }
    for (auto& t : threads)
        t.join();

    return static_cast<double>(hits) / N * 4;
}

//---------------------------------------------------------------------------------------
// Monte-Carlo Pi with Mutex

void calculate_hits_atomic_hot_loop(uintmax_t N, std::atomic<uintmax_t>& hits)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    for (uintmax_t n = 0; n < N; ++n)
    {
        double x = dist(gen);
        double y = dist(gen);
        if (x * x + y * y <= 1)
        {
            //++hits;
            hits.fetch_add(1, std::memory_order_relaxed); 
        }
    }
}

double mc_pi_multi_thread_atomic(uintmax_t N)
{
    std::atomic<uintmax_t> hits = 0;
    
    int num_cores = std::thread::hardware_concurrency();

    std::vector<std::thread> threads;
    std::vector<uintmax_t> results(num_cores, 0);

    for (int i = 0; i < num_cores; ++i)
    {
        //std::cout << "Launching thread " << i << std::endl;
        threads.push_back(std::thread([&]{
            calculate_hits_atomic_hot_loop(N / num_cores, hits);
        }));
    }
    for (auto& t : threads)
        t.join();

    return static_cast<double>(hits) / N * 4;
}

TEST_CASE("Monte-Carlo Pi - Multi Thread with Atomic")
{
    SECTION("Calculate Pi using multiple threads with mutex")
    {
        auto result = mc_pi_multi_thread_atomic(N);
        std::cout << "Calculated Pi: " << result << std::endl;
    }

    BENCHMARK("Monte Carlo Pi - Multi Thread with Mutex")
    {
        return mc_pi_multi_thread_atomic(N);
    };
}

TEST_CASE("Monte-Carlo Pi - Multi Thread with Mutex")
{
    SECTION("Calculate Pi using multiple threads with mutex")
    {
        auto result = mc_pi_multi_thread_mutex(N);
        std::cout << "Calculated Pi: " << result << std::endl;
    }

    BENCHMARK("Monte Carlo Pi - Multi Thread with Mutex")
    {
        return mc_pi_multi_thread_mutex(N);
    };
}

TEST_CASE("Monte-Carlo Pi - Single Thread")
{
    SECTION("Calculate Pi using single thread")
    {
        auto result = mc_pi_single_thread(N);
        std::cout << "Calculated Pi: " << result << std::endl;
    }

    BENCHMARK("Monte Carlo Pi - Single Thread")
    {
        return mc_pi_single_thread(N);
    };
}

TEST_CASE("Monte-Carlo Pi - Many Threads")
{
    SECTION("Calculate Pi using many threads")
    {
        auto result = mc_pi_multi_thread(N);
        std::cout << "Calculated Pi: " << result << std::endl;
    }

    BENCHMARK("Monte Carlo Pi - Many Threads")
    {
        return mc_pi_multi_thread(N);
    };
}

TEST_CASE("Monte-Carlo Pi - Many Threads with Local Counter")
{
    SECTION("Calculate Pi using many threads with local counter")
    {
        auto result = mc_pi_multi_thread_with_local_counter(N);
        std::cout << "Calculated Pi: " << result << std::endl;
    }

    BENCHMARK("Monte Carlo Pi - Many Threads with Local Counter")
    {
        return mc_pi_multi_thread_with_local_counter(N);
    };
}

TEST_CASE("Monte-Carlo Pi - Many Threads with Aligned Counter")
{
    SECTION("Calculate Pi using many threads with aligned counter")
    {
        auto result = mc_pi_multi_thread_with_aligned_counter(N);
        std::cout << "Calculated Pi: " << result << std::endl;
    }

    BENCHMARK("Monte Carlo Pi - Many Threads with Aligned Counter")
    {
        return mc_pi_multi_thread_with_aligned_counter(N);
    };
}

//////////////////////////////////////////////////////////////////
uintmax_t calculate_hits_future(uintmax_t N)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    uintmax_t local_hits = 0;
    for (uintmax_t n = 0; n < N; ++n)
    {
        double x = dist(gen);
        double y = dist(gen);
        if (x * x + y * y <= 1)
            ++local_hits;
    }

    return local_hits;
}

double mc_pi_multi_thread_future(uintmax_t N, uintmax_t num_threads = std::max(1u, std::thread::hardware_concurrency()))
{
    uintmax_t chunk_size = N / num_threads;

    std::vector<std::future<uintmax_t>> futures;
    for (uintmax_t i = 0; i < num_threads; ++i)
    {
        futures.emplace_back(std::async(std::launch::async, calculate_hits_future, chunk_size));
    }

    uintmax_t total_hits = 0;
    for (auto& future : futures)
    {
        total_hits += future.get();
    }

    return static_cast<double>(total_hits) / N * 4;
}

TEST_CASE("Monte-Carlo Pi - Many Threads with Future")
{
    SECTION("Calculate Pi using many threads with future")
    {
        auto result = mc_pi_multi_thread_future(N);
        std::cout << "Calculated Pi: " << result << std::endl;
    }

    BENCHMARK("Monte Carlo Pi - Many Threads with Future")
    {
        return mc_pi_multi_thread_future(N);
    };
}