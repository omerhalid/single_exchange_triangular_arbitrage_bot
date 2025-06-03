#include "triarb_bot.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <iomanip>
#include <unordered_map>

using json = nlohmann::json;

namespace triarb {

TriArbBot::TriArbBot(boost::asio::io_context& ioc)
    : session_(ioc,
               "stream.binance.com",
               "9443",
               "/stream?streams=btcusdt@depth5@100ms/ethbtc@depth5@100ms/ethusdt@depth5@100ms",
               [this](std::string_view msg) { handle_frame(msg); }) {}

void TriArbBot::start() { session_.run(); }

void TriArbBot::print_book_update(const std::string& symbol, const OrderBook& book)
{
    static std::unordered_map<std::string, Quote> lastBids;
    static std::unordered_map<std::string, Quote> lastAsks;

    const auto bestBid = book.bestBid();
    const auto bestAsk = book.bestAsk();

    if(bestBid.px != lastBids[symbol].px || bestBid.qty != lastBids[symbol].qty ||
       bestAsk.px != lastAsks[symbol].px || bestAsk.qty != lastAsks[symbol].qty)
    {
        lastBids[symbol] = bestBid;
        lastAsks[symbol] = bestAsk;
        auto precision = (symbol == "ETH-BTC") ? 8 : 2;
        std::cout << std::fixed << std::setprecision(precision)
                  << symbol << " - "
                  << "Best bid: " << bestBid.px << " (" << bestBid.qty << ")\n"
                  << "Best ask: " << bestAsk.px << " (" << bestAsk.qty << ")\n"
                  << "Spread: " << (bestAsk.px - bestBid.px) << "\n\n";
    }
}

bool TriArbBot::edge_scanner()
{
    if(!got_btc_ || !got_ethbtc_ || !got_ethusdt_)
        return false;

    if (eth_usdt_book_.bestAsk().px <= 0 ||
        eth_btc_book_.bestBid().px <= 0 ||
        btc_usdt_book_.bestBid().px <= 0)
        return false;

    double maker = -0.0001;
    double taker =  0.0004;

    auto edge =
        (1.0)
        /  eth_usdt_book_.bestAsk().px * (1.0 + taker)
        *  eth_btc_book_.bestBid().px * (1.0 - maker)
        *  btc_usdt_book_.bestBid().px * (1.0 - maker)
        - 1.0;

    constexpr double EDGE_THRESHOLD = 0.0008;
    if(edge > EDGE_THRESHOLD && std::abs(edge - last_edge_) > 1e-6)
    {
        last_edge_ = edge;
        std::cout << "Edge " << edge*100 << "% -> FIRE\n";
        return true;
    }
    last_edge_ = edge;
    return false;
}

void TriArbBot::handle_frame(std::string_view msg)
{
    try {
        static int msg_count = 0;
        if (++msg_count % 100 == 0)
            std::cout << "Processed " << msg_count << " messages\n";

        auto j = json::parse(msg);

        if (!j.contains("stream") || !j["stream"].is_string())
            return;
        if (!j.contains("data") || !j["data"].is_object())
            return;
        auto& data = j["data"];
        auto stream = j["stream"].get<std::string>();
        auto id = data["lastUpdateId"].get<uint64_t>();
        if (!data.contains("bids") || !data["bids"].is_array() || data["bids"].empty())
            return;
        if (!data.contains("asks") || !data["asks"].is_array() || data["asks"].empty())
            return;

        Quote bid {
            std::stod(data["bids"][0][0].get<std::string>()),
            std::stod(data["bids"][0][1].get<std::string>())
        };
        Quote ask {
            std::stod(data["asks"][0][0].get<std::string>()),
            std::stod(data["asks"][0][1].get<std::string>())
        };

        if (stream == "btcusdt@depth5@100ms") {
            btc_usdt_book_.update(id, bid, ask);
            print_book_update("BTC-USDT", btc_usdt_book_);
            got_btc_ = true;
        } else if (stream == "ethbtc@depth5@100ms") {
            eth_btc_book_.update(id, bid, ask);
            print_book_update("ETH-BTC", eth_btc_book_);
            got_ethbtc_ = true;
        } else if (stream == "ethusdt@depth5@100ms") {
            eth_usdt_book_.update(id, bid, ask);
            print_book_update("ETH-USDT", eth_usdt_book_);
            got_ethusdt_ = true;
        }

        if (edge_scanner())
            std::cout << "Arbitrage opportunity found!\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

} // namespace triarb


