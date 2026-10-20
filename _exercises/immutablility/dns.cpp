#include <iostream>
#include <thread>
#include <unordered_map>
#include <optional>
#include <chrono>

using IpAddress = std::string;
using Hostname = std::string;

// TODO: Implement Dns as immutable

class Dns
{
    std::unordered_map<Hostname, IpAddress> dns_table_;

public:
    Dns() = default;

    Dns(std::unordered_map<Hostname, IpAddress> table)
        : dns_table_{std::move(table)}
    {
    }

    bool add_record(const Hostname& hostname, const IpAddress& ip)
    {
        auto result = dns_table_.insert_or_assign(hostname, ip);
        return result.second;
    }

    std::optional<IpAddress> resolve(const Hostname& hostname) const
    {
        auto it = dns_table_.find(hostname);
        if (it != dns_table_.end())
            return it->second;
        return std::nullopt;
    }
};

void dns_client(std::shared_ptr<const Dns> dns, std::chrono::milliseconds delay)
{
    const auto entires_to_resolve = {
        "example.com",
        "example.org",
        "example.net",
        "thread-safety.com",
        "thread-safety.org"
    };

    for (const auto& hostname : entires_to_resolve)
    {
        if (auto ip = dns->resolve(hostname); ip)
            std::cout << hostname << " -> " << *ip << "\n";
        else
            std::cout << hostname << " not found\n";

        std::this_thread::sleep_for(delay);
    }
}

int main()
{
    using namespace std::literals;

    // TODO: Create an immutable Dns instance that safely lives throughout the lifetime of the threads


    std::jthread dns_client_thd_1;

    {
        std::shared_ptr<const Dns> dns = []{ 
            auto dns = std::make_shared<Dns>();
            dns->add_record("example.com", "93.184.216.34");
            dns->add_record("example.org", "93.184.216.34");
            dns->add_record("example.net", "93.184.216.34");
            dns->add_record("thread-safety.com", "93.184.216.34");
            dns->add_record("thread-safety.org", "93.184.216.34");
            return dns;
        }();

        std::jthread dns_client_thd_2(dns_client, dns, 100ms);
        dns_client_thd_1 = std::jthread(dns_client, dns, 500ms);   
    }
}
