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

            // Create Quote objects from JSON
            Quote bid{
                std::stod( j["bids"][0][0].get<std::string>() ),
                std::stod( j["bids"][0][1].get<std::string>() )
            };
            Quote ask{
                std::stod( j["asks"][0][0].get<std::string>() ),
                std::stod( j["asks"][0][1].get<std::string>() )
            };

            if(stream == "btcusdt@depth5")
            {
                btc_usdt_book.update(j["lastUpdateId"].get<uint64_t>(), bid, ask);
                printBookUpdate("BTC-USDT", btc_usdt_book);
            }
            else if(stream == "ethbtc@depth5")
            {
                eth_btc_book.update(j["lastUpdateId"].get<uint64_t>(), bid, ask);
                printBookUpdate("BTC-USDT", eth_btc_book);
            }
            else if(stream == "ethusdt@depth5")
            {
                eth_usdt_book.update(j["lastUpdateId"].get<uint64_t>(), bid, ask);
                printBookUpdate("BTC-USDT", eth_usdt_book);
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
    const auto bestBid = book.bestBid();
    const auto bestAsk = book.bestAsk();

    if(bestBid.px != lastBid.px || bestBid.qty != lastBid.qty || 
        bestAsk.px != lastAsk.px || bestAsk.qty != lastAsk.qty)
    {
        lastBid = bestBid;
        lastAsk = bestAsk;

        std::cout << std::fixed << std::setprecision(2)
            << "Best bid: $" << bestBid.px << " (" << bestBid.qty << symbol<<"\n"
            << "Best ask: $" << bestAsk.px << " (" << bestAsk.qty << symbol<<"\n"
            << "Spread: $" << (bestAsk.px - bestBid.px)
            << "\n\n";
        
    }
}
