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
    for (uintmax_t n = 0; n < N; ++n)
    {
        double x = rand() / static_cast<double>(RAND_MAX);
        double y = rand() / static_cast<double>(RAND_MAX);
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

TEST_CASE("Monte-Carlo Pi - Single Thread")
{
    BENCHMARK("Monte Carlo Pi - Single Thread")
    {
        return mc_pi_single_thread(N);
    };
}