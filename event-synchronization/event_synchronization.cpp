#include <cassert>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

namespace BusyWait
{
    class Data
    {
        std::vector<int> data_;
        std::atomic<bool> is_data_ready_ = false;
        double value_{};

    public:
        void read()
        {
            std::cout << "Start reading..." << std::endl;
            data_.resize(100);

            std::random_device rnd;
            std::generate(begin(data_), end(data_), [&rnd] { return rnd() % 1000; });
            std::this_thread::sleep_for(2s);

            std::cout << "End reading..." << std::endl;

            // is_data_ready_ = true;
            data_.push_back(665);

            // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
            is_data_ready_.store(true, std::memory_order_release);
        }

        void process(int id)
        {
            while (not is_data_ready_.load(std::memory_order_acquire))
            // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

            { }

            long sum = std::accumulate(begin(data_), end(data_), 0L);
            std::cout << "Id: " << id << "; Sum: " << sum << std::endl;
        }
    };
} // namespace BusyWait

namespace IdleWait
{
    class Data
    {
        std::vector<int> data_;
        double value_{};
        bool is_data_ready_ = false;
        std::mutex mtx_;
        std::condition_variable cv_is_data_ready_;

    public:
        void read()
        {
            std::cout << "Start reading..." << std::endl;
            data_.resize(100);

            std::random_device rnd;
            std::generate(begin(data_), end(data_), [&rnd] { return rnd() % 1000; });
            std::this_thread::sleep_for(2s);

            std::cout << "End reading..." << std::endl;

            data_.push_back(665);

            {
                std::lock_guard lk{mtx_};
                is_data_ready_ = true;
            }

            cv_is_data_ready_.notify_all();
        }

        void process(int id)
        {
            {
                std::unique_lock lk{mtx_};
                // while (not is_data_ready_)
                // {
                //     cv_is_data_ready_.wait(lk);
                // }

                cv_is_data_ready_.wait(lk, [this] { return is_data_ready_; });
            }

            long sum = std::accumulate(begin(data_), end(data_), 0L);
            std::cout << "Id: " << id << "; Sum: " << sum << std::endl;
        }
    };
} // namespace IdleWait

int main()
{
    using Data = IdleWait::Data;

    {
        Data data;
        std::jthread thd_producer{[&data] { data.read(); }};

        std::jthread thd_consumer_1{[&data] { data.process(1); }};
        std::jthread thd_consumer_2{[&data] { data.process(2); }};
    }

    std::cout << "END of main..." << std::endl;
}
