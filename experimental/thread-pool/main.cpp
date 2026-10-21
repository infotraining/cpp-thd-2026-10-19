#include "concurrent_queue.hpp"

#include <iostream>
#include <thread>

using namespace std;
using namespace ThreadLab;

int main()
{
    ConcurrentQueue<int> cq;

    {
        auto producer_thd = std::jthread([&cq] {
            for (int i = 0; i < 10; ++i)
            {
                std::cout << "Produced: " << i << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
                cq.push(i);
            }
            cq.close();
        });

        auto consumer_thd = std::jthread([&cq] {
            while (true)
            {
                std::optional<int> item = cq.pop();
                if (!item.has_value())
                {
                    break;
                }
                std::cout << "Consumed: " << item.value() << std::endl;
            }
        });
    }

    std::cout << "All threads have finished." << std::endl;
}
