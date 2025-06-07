#pragma once

#include "websocket_session.hpp"
#include "gateway.hpp"
#include "orderbook.hpp"
#include <boost/asio.hpp>
#include <atomic>
#include <string_view>
#include <cstdlib> 

namespace triarb {

class TriArbBot {
public:
    explicit TriArbBot(boost::asio::io_context& ioc);
    void start();

private:
    void handle_frame(std::string_view msg);
    void print_book_update(const std::string& symbol, const OrderBook& book);
    bool edge_scanner();

    OrderBook btc_usdt_book_{"BTCUSDT"};
    OrderBook eth_btc_book_{"ETHBTC"};
    OrderBook eth_usdt_book_{"ETHUSDT"};

    std::atomic_bool got_btc_{false};
    std::atomic_bool got_ethbtc_{false};
    std::atomic_bool got_ethusdt_{false};

    double last_edge_ = 0.0;
    WebsocketSession session_;
};

} // namespace triarb


