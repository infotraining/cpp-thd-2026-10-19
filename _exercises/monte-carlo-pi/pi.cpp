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

const uintmax_t N = 1'000'000;

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
        if (x * x + y * y <= 1)
            ++local_hits;
    }

    hits += local_hits;
}

double mc_pi_single_thread(uintmax_t N)
{
    uintmax_t hits = 0;
    calculate_hits(N, hits);

    return static_cast<double>(hits) / N * 4;
}

//Dominik
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