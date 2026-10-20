#include <chrono>
#include <iostream>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <thread>
#include <unordered_map>

using IpAddress = std::string;
using Hostname = std::string;

// TODO: Implement Dns as immutable

namespace Immutable
{
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
} // namespace Immutable

namespace ReadersWriters
{
    class Dns
    {
        std::unordered_map<Hostname, IpAddress> dns_table_;
        mutable std::shared_mutex mtx_dns_table_;

    public:
        Dns() = default;

        Dns(std::unordered_map<Hostname, IpAddress> table)
            : dns_table_{std::move(table)}
        {
        }

        bool add_record(const Hostname& hostname, const IpAddress& ip)
        {
            std::unique_lock lk_writer(mtx_dns_table_);
            auto result = dns_table_.insert_or_assign(hostname, ip);
            return result.second;
        }

        std::optional<IpAddress> resolve(const Hostname& hostname) const
        {
            std::shared_lock lk_reader(mtx_dns_table_);
            auto it = dns_table_.find(hostname);
            if (it != dns_table_.end())
                return it->second;
            return std::nullopt;
        }
    };

} // namespace ReadersWriters

void dns_client(std::shared_ptr<const Immutable::Dns> dns, std::chrono::milliseconds delay)
{
    const auto entires_to_resolve = {
        "example.com",
        "example.org",
        "example.net",
        "thread-safety.com",
        "thread-safety.org"};

    for (const auto& hostname : entires_to_resolve)
    {
        if (auto ip = dns->resolve(hostname); ip)
            std::cout << hostname << " -> " << *ip << "\n";
        else
            std::cout << hostname << " not found\n";

        std::this_thread::sleep_for(delay);
    }
}

void dns_client_rw(std::shared_ptr<ReadersWriters::Dns> dns, std::chrono::milliseconds delay)
{
    const auto entires_to_resolve = {
        "example.com",
        "example.org",
        "example.net",
        "thread-safety.com",
        "thread-safety.org"};

    for (const auto& hostname : entires_to_resolve)
    {
        if (auto ip = dns->resolve(hostname); ip)
            std::cout << hostname << " -> " << *ip << "\n";
        else
            std::cout << hostname << " not found\n";

        std::this_thread::sleep_for(delay);
    }
}

void immutable()
{
    using namespace std::literals;

    // TODO: Create an immutable Dns instance that safely lives throughout the lifetime of the threads

    std::jthread dns_client_thd_1;

    {
        std::shared_ptr<const Immutable::Dns> dns = [] {
            auto dns = std::make_shared<Immutable::Dns>();
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

void readers_writers()
{
    using namespace std::literals;

    // TODO: Create an immutable Dns instance that safely lives throughout the lifetime of the threads

    std::jthread dns_client_thd_1;

    {
        auto dns = std::make_shared<ReadersWriters::Dns>();
        dns->add_record("example.com", "93.184.216.34");
        dns->add_record("example.org", "93.184.216.34");
        dns->add_record("example.net", "93.184.216.34");
        
        std::jthread dns_client_thd_2(dns_client_rw, dns, 100ms);

        dns->add_record("thread-safety.com", "93.184.216.34");
        dns->add_record("thread-safety.org", "93.184.216.34");
        
        dns_client_thd_1 = std::jthread(dns_client_rw, dns, 500ms);
    }
}

int main()
{
    immutable();

    std::cout << "-------------" << std::endl;

    readers_writers();
}