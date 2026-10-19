#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <variant>

using namespace std::literals;

template <typename T>
class Result
{
    std::variant<std::monostate, T, std::exception_ptr> value_;

public:
    template <typename TValue>
    void set_value(TValue&& value)
    {
        value_ = std::forward<TValue>(value);
    }

    void set_exception(std::exception_ptr eptr)
    {
        value_ = eptr;
    }

    T get()
    {
        struct ValueVisitor
        {
            T operator()(std::monostate)
            {
                throw std::logic_error("Result/Exception not set");
            }

            T operator()(T& v) const
            {
                return std::move(v);
            }

            T operator()(std::exception_ptr eptr)
            {
                std::rethrow_exception(eptr);
            }
        };

        return std::visit(ValueVisitor{}, value_);
    }
};

namespace BeforeRefactoring
{
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
        catch (...)
        {
            eptr = std::current_exception();
            std::cout << "Exception in bw#" << id << "..." << std::endl;
            return;
        }

        std::cout << "bw#" << id << " is finished..." << std::endl;
    }
} // namespace BeforeRefactoring

void background_work(size_t id, const std::string& text, Result<char>& result)
{
    std::cout << "bw#" << id << " has started..." << std::endl;

    for (const auto& c : text)
    {
        std::cout << "bw#" << id << ": " << c << std::endl;

        std::this_thread::sleep_for(100ms);
    }

    try
    {
        result.set_value(text.at(5)); // potential exception
    }
    catch (...)
    {
        result.set_exception(std::current_exception());
        std::cout << "Exception in bw#" << id << "..." << std::endl;
        return;
    }

    std::cout << "bw#" << id << " is finished..." << std::endl;
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    Result<char> result_1;
    Result<char> result_2;

    {
        std::jthread thd_1{&background_work, 1, text, std::ref(result_1)};
        std::jthread thd_2{&background_work, 2, std::string("One"), std::ref(result_2)};
    }

    try
    {
        char c1 = result_1.get();
        std::cout << "result_1: " << c1 << "\n";

        char c2 = result_2.get();
        std::cout << "result_2: " << c2 << "\n";
    }
    catch(const std::out_of_range& e)
    {
        std::cout << "Caught exception: " << e.what() << "\n";
    }

    std::cout << "Main thread ends..." << std::endl;
}
