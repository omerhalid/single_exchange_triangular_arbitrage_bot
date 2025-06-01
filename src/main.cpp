#include "common.hpp"
#include "websocket_session.hpp"
#include "orderbook.hpp"
#include <iostream>
#include <boost/asio.hpp>
#include <unordered_map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void printBookUpdate(const std::string& symbol, const OrderBook& book)
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

        // Add different precision for ETH-BTC pair
        auto precision = (symbol == "ETH-BTC") ? 8 : 2;

        std::cout << std::fixed << std::setprecision(precision)
            << symbol << " - "  // Added pair name to output
            << "Best bid: " << bestBid.px << " (" << bestBid.qty << ")\n"
            << "Best ask: " << bestAsk.px << " (" << bestAsk.qty << ")\n"
            << "Spread: " << (bestAsk.px - bestBid.px)
            << "\n\n";
    }
}

int main() {
    std::cout << "Hello, Binance! running " << version << '\n';

    boost::asio::io_context ioc;

    OrderBook btc_usdt_book("BTCUSDT");
    OrderBook eth_btc_book("ETHBTC");
    OrderBook eth_usdt_book("ETHUSDT");

    std::cout << "Creating WebSocket connection...\n";  // Add debug print

    WebsocketSession session(
    ioc,
    "stream.binance.com",
    "9443",
    "/stream?streams=btcusdt@depth5@100ms/ethbtc@depth5@100ms/ethusdt@depth5@100ms",
    [&](std::string_view msg) {
        try {
            auto j = json::parse(msg);

            // 1) Ensure it has a "stream" field:
            if (!j.contains("stream") || !j["stream"].is_string())
                return; // not a combined‐streams data message

            // 2) Ensure "data" is an object:
            if (!j.contains("data") || !j["data"].is_object())
                return;

            auto& data = j["data"];

            // 3) Ensure "bids"/"asks" exist and are non‐empty arrays:
            if (!data.contains("bids") || !data["bids"].is_array() || data["bids"].empty())
                return;
            if (!data.contains("asks") || !data["asks"].is_array() || data["asks"].empty())
                return;

            // 4) Now grab the top‐of‐book quotes safely:
            Quote bid {
                std::stod(data["bids"][0][0].get<std::string>()),
                std::stod(data["bids"][0][1].get<std::string>())
            };
            Quote ask {
                std::stod(data["asks"][0][0].get<std::string>()),
                std::stod(data["asks"][0][1].get<std::string>())
            };
            
            auto stream = j["stream"].get<std::string>();

            if (stream == "btcusdt@depth5@100ms") {
                btc_usdt_book.update(
                  data["lastUpdateId"].get<uint64_t>(), bid, ask);
                printBookUpdate("BTC-USDT", btc_usdt_book);
            }
            else if (stream == "ethbtc@depth5@100ms") {
                eth_btc_book.update(
                  data["lastUpdateId"].get<uint64_t>(), bid, ask);
                printBookUpdate("ETH-BTC", eth_btc_book);
            }
            else if (stream == "ethusdt@depth5@100ms") {
                eth_usdt_book.update(
                  data["lastUpdateId"].get<uint64_t>(), bid, ask);
                printBookUpdate("ETH-USDT", eth_usdt_book);
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        }
    });

            
    std::cout << "Starting WebSocket...\n";  // Add debug print
    session.run();

    std::cout << "Running IO context...\n";  // Add debug print
    ioc.run();

    return 0;
}
