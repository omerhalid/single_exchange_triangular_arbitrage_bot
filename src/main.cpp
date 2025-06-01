#include "common.hpp"
#include "websocket_session.hpp"
#include "orderbook.hpp"
#include <iostream>
#include <boost/asio.hpp>
#include <unordered_map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
    std::cout << "Hello, Binance! running " << version << '\n';

    boost::asio::io_context ioc;

    OrderBook btc_usdt_book("BTCUSDT");
    OrderBook eth_btc_book("ETHBTC");
    OrderBook eth_usdt_book("ETHUSDT");

    WebsocketSession session(
        ioc,
        "stream.binance.com",
        "9443",
        "/ws/btcusdt@depth5@100ms/ethbtc@depth5@100ms/ethusdt@depth5@100ms",
        [&](std::string_view msg) {

            // Parse JSON first
            auto j = json::parse(msg);

            // Get the stream name to identify which pair updated
            std::string stream = j["stream"].get<std::string>();
            const auto& data = j["data"];

            Quote bid{
                std::stod(data["bids"][0][0].get<std::string>()),
                std::stod(data["bids"][0][1].get<std::string>())
            };
            Quote ask{
                std::stod(data["asks"][0][0].get<std::string>()),
                std::stod(data["asks"][0][1].get<std::string>())
            };

            if(stream == "btcusdt@depth5") {
                btc_usdt_book.update(data["lastUpdateId"].get<uint64_t>(), bid, ask);
                printBookUpdate("BTC-USDT", btc_usdt_book);
            }
            else if(stream == "ethbtc@depth5") {
                eth_btc_book.update(data["lastUpdateId"].get<uint64_t>(), bid, ask);
                printBookUpdate("ETH-BTC", eth_btc_book);  // Fixed symbol
            }
            else if(stream == "ethusdt@depth5") {
                eth_usdt_book.update(data["lastUpdateId"].get<uint64_t>(), bid, ask);
                printBookUpdate("ETH-USDT", eth_usdt_book);  // Fixed symbol
            }
            
        });
            
    // Resolve DNS + Connect + Handshake (starts the async chain)
    session.run();

    // Run the I/O loop (this blocks until the program is terminated or an error occurs)
    ioc.run();

    return 0;
}

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
