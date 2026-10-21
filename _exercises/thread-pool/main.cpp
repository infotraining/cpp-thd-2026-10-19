#include <chrono>
#include <iostream>
#include <random>
#include <syncstream>
#include <future>

using namespace std;

#include "thread_pool.hpp"

std::osyncstream synced_cout()
{
    return std::osyncstream(std::cout);
}

void background_work(size_t id, const std::string& text, std::chrono::milliseconds delay)
{
    synced_cout() << "bw#" << id << " has started..." << std::endl;

    for (const auto& c : text)
    {
        synced_cout() << "bw#" << id << ": " << c
                      << " in Thread#" << std::this_thread::get_id() << std::endl;

        std::this_thread::sleep_for(delay);
    }

    synced_cout() << "bw#" << id << " is finished..." << std::endl;
}

int calculate_square(int x)
{
    synced_cout() << "Starting calculation for " << x << " in " << std::this_thread::get_id() << std::endl;

    std::random_device rd;
    std::uniform_int_distribution<> distr(100, 5000);

    std::this_thread::sleep_for(std::chrono::milliseconds(distr(rd)));

    if (x % 3 == 0)
        throw std::runtime_error("Error#3");

    return x * x;
}

int main()
{
    ThreadPool thd_pool{4};

    for (size_t i = 1; i < 20; ++i)
    {
        thd_pool.submit([i] { background_work(i, "BW#" + std::to_string(i), 100ms); });
    }

    std::vector<std::future<int>> f_squares;

    for (size_t i = 1; i < 20; ++i)
    {
        f_squares.push_back(thd_pool.submit([i] { return calculate_square(i); }));
    }

    for (auto& fs : f_squares)
    {
        try
        {
            int result = fs.get();
            synced_cout() << "Result: " << result << "\n";
        }
        catch (const std::exception& e)
        {
            synced_cout() << "Exception: " << e.what() << '\n';
        }
    }
}
