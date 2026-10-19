#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

void background_work(size_t id, const std::string& text, std::chrono::milliseconds delay)
{
    std::cout << "bw#" << id << " has started..." << std::endl;

    for (const auto& c : text)
    {
        std::cout << "bw#" << id << ": " << c << std::endl;

        std::this_thread::sleep_for(delay);
    }

    std::cout << "bw#" << id << " is finished..." << std::endl;
}

class BackgroundWork
{
    const int id_;
    const std::string text_;

public:
    BackgroundWork(int id, std::string text)
        : id_{id}
        , text_{std::move(text)}
    {
    }

    void operator()(std::chrono::milliseconds delay) const
    {
        std::cout << "BW#" << id_ << " has started..." << std::endl;

        for (const auto& c : text_)
        {
            std::cout << "BW#" << id_ << ": " << c << std::endl;

            std::this_thread::sleep_for(delay);
        }

        std::cout << "BW#" << id_ << " is finished..." << std::endl;
    }
};

std::thread create_thread()
{
    static uint32_t id = 1000;
    ++id;

    std::string text = "THREAD#" + std::to_string(id);

    std::thread thd(&background_work, id, text, 100ms);

    try
    {
        std::cout << text.at(100) << std::endl;
    }
    catch(...)
    {
        thd.join();
        throw;
    }

    return thd;
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    std::thread empty_thd;
    std::cout << "empty_thd: " << empty_thd.get_id() << std::endl;

    std::vector<std::thread> threads; // group of threads

    std::thread thd_1(&background_work, 1, "Thread#1", 250ms);
    std::thread thd_2(&background_work, 2, std::cref(text), 450ms);

    threads.push_back(std::move(thd_1));
    threads.push_back(std::move(thd_2));

    BackgroundWork bw_1(3, "BackgroundWork#3");
    std::thread thd_3(std::cref(bw_1), 303ms);
    threads.push_back(std::move(thd_3));

    try
    {
        threads.push_back(create_thread());
        threads.push_back(create_thread());
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    for(auto& thd : threads)
    {
        if (thd.joinable())
            thd.join();
    }

    std::cout << "Main thread ends..." << std::endl;
}