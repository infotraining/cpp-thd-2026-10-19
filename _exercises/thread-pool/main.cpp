#include <iostream>
#include <chrono>
#include <syncstream>

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
            << " in Thread#" << std::this_thread::get_id() << 
         std::endl;

        std::this_thread::sleep_for(delay);
    }

    synced_cout() << "bw#" << id << " is finished..." << std::endl;
}

int main()
{
    ThreadPool thd_pool{4};

    for(size_t i = 1; i < 20; ++i)
    {
        thd_pool.submit([i]{ background_work(i, "BW#" + std::to_string(i), 100ms); });
    }
}
