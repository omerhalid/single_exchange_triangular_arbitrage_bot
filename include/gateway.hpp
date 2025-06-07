#pragma once
#include <functional>
#include <string>
#include <boost/asio.hpp>
#include <iostream>
#include <iomanip>      
#include <sstream>      

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

        /* Order Sender
        * -----------
        * Sends a new order to Binance API with specified parameters.
        * Supports both live trading and dry-run simulation mode.
        * 
        * Parameters:
        * - symbol:  Trading pair (e.g., "BTCUSDT")
        * - side:    Trade direction ("BUY" or "SELL")
        * - qty:     Order quantity in base asset
        * - price:   Limit order price
        * - cb:      Callback function that receives FillReport
        *
        * Trading Configuration:
        * - Uses LIMIT_MAKER order type to ensure maker fees
        * - IOC (Immediate-or-Cancel) time in force
        * - 5000ms receive window for API
        * 
        * Example:
        * gateway.send_order("BTCUSDT", "BUY", 0.001, 50000.00,
        *     [](FillReport report) {
        *         if(report.success) {
        *             std::cout << "Filled " << report.qty_filled 
        *                      << " @ " << report.price_avg << "\n";
        *         }
        *     });
        */
        void send_order(
            std::string_view symbol,
            std::string_view side, // BUY / SELL
            double qty,
            double price,
            std::function<void(FillReport)> cb
        );
            
    private:

        std::string host_;
        boost::asio::io_context& ioc_;
        bool live_;
        ApiKeys keys_;
        boost::asio::ssl::context ctx_;
};
