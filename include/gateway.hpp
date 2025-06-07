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
    bool   success;      // got ≥1 fill?
    double qty_filled;   // base asset
    double price_avg;    // VWAP
};

class Gateway
{
    public:
        Gateway(boost::asio::io_context& ioc,
                std::string              rest_host,   // "api.binance.com"
                ApiKeys                  keys,
                bool                     live
            );

        void send_order(
            std::string_view symbol,
            std::string_view side, // BUY / SELL
            double qty,
            double price,
            std::function<void(FillReport)> cb
        );
            
    private:

        std::string host_;
        boost::asio::io_context ioc_;
        bool live_;
        ApiKeys keys_;
        boost::asio::ssl::context ctx_
};
