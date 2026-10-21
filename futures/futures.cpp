#include <cassert>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <random>
#include <string>
#include <syncstream>
#include <thread>
#include <vector>

using namespace std::literals;

std::osyncstream synced_cout()
{
    return std::osyncstream(std::cout);
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

void save_to_file(const std::string& filename)
{
    synced_cout() << "Saving to file: " << filename << std::endl;

    std::this_thread::sleep_for(3s);

    synced_cout() << "File saved: " << filename << std::endl;
}

int main()
{
    std::future<int> f_square_13 = std::async(std::launch::async, &calculate_square, 13);
    std::future<int> f_square_17 = std::async(std::launch::async, [] { return calculate_square(17); });
    std::future<int> f_square_9 = std::async(std::launch::deferred, [] { return calculate_square(9); });

    std::future<void> f_save = std::async(std::launch::async, &save_to_file, "data.txt");

    while (f_save.wait_for(100ms) != std::future_status::ready)
    {
        synced_cout() << "." << std::flush_emit;
    }

    synced_cout() << "Result square(13) = " << f_square_13.get() << std::endl;
    synced_cout() << "Result square(17) = " << f_square_17.get() << std::endl;

    try
    {
        int result = f_square_9.get();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    synced_cout() << "-----------------------------\n";

    auto f1 = std::async(std::launch::async, &save_to_file, "data1.txt");
    auto f2 = std::async(std::launch::async, &save_to_file, "data2.txt");
    auto f3 = std::async(std::launch::async, &save_to_file, "data3.txt");

    synced_cout() << "-----------------------------\n";

    std::vector<std::future<int>> f_squares;

    for (int i = 1; i <= 10; ++i)
    {
        f_squares.push_back(std::async(std::launch::async, &calculate_square, i));
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
