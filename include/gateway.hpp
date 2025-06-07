#pragma once
#include <functional>
#include <string>
#include <boost/asio.hpp>

struct ApiKeys
{
    std::string key;
    std::string secret;
};

struct FillReport
{

};

class Gateway
{
    public:
        Gateway(boost::asio::io_context& ioc,
                std::string              rest_host,   // "api.binance.com"
                ApiKeys                  keys,
                bool                     live
            );
    private:
};