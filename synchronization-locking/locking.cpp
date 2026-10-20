#include <atomic>
#include <barrier>
#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

void may_throw()
{
}

void run(int& value, std::mutex& mtx_value)
{
    for (int i = 0; i < 10'000'000; ++i)
    {
        {
            std::lock_guard<std::mutex> lk{mtx_value}; // mtx_value.lock()
            ++value;
            may_throw();
        }
    } // mtx_value.unlock()
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;

    int counter = 0;
    std::mutex mtx_counter;

    {
        std::jthread thd_1{[&counter, &mtx_counter] { run(counter, mtx_counter); }};
        std::jthread thd_2{&run, std::ref(counter), std::ref(mtx_counter)};
    }

    std::cout << "counter: " << counter << "\n";

    std::cout << "Main thread ends..." << std::endl;
}
