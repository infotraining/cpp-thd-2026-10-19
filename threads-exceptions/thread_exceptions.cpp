#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

void background_work(size_t id, const std::string& text, char& result, std::exception_ptr& eptr)
{
    std::cout << "bw#" << id << " has started..." << std::endl;

    for (const auto& c : text)
    {
        std::cout << "bw#" << id << ": " << c << std::endl;

        std::this_thread::sleep_for(100ms);
    }

    try
    {
         result = text.at(5); // potential exception
    }
    catch(...)
    {
        eptr = std::current_exception();
        std::cout << "Exception in bw#" << id << "..." << std::endl;
        return;
    }

    std::cout << "bw#" << id << " is finished..." << std::endl;
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    char result_1{};
    std::exception_ptr eptr_1;
    
    char result_2{};
    std::exception_ptr eptr_2;
    
    {
        std::jthread thd_1{&background_work, 1, text, std::ref(result_1), std::ref(eptr_1)};
        std::jthread thd_2{&background_work, 2, std::string("One"), std::ref(result_2), std::ref(eptr_2)};
    }

    if (eptr_1)
    {
        try
        {
            std::rethrow_exception(eptr_1);
        }
        catch(const std::out_of_range& e)
        {
            std::cout << "Handling " << e.what() << std::endl;
        }
    }

    if (eptr_2)
    {
        try
        {
            std::rethrow_exception(eptr_2);
        }
        catch(const std::out_of_range& e)
        {
            std::cout << "Handling " << e.what() << std::endl;
        }
    }

    std::cout << "Main thread ends..." << std::endl;
}
