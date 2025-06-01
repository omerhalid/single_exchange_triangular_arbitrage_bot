#include "common.hpp"
#include "websocket_session.hpp"
#include "orderbook.hpp"
#include <iostream>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
    std::cout << "Hello, Binance! running " << version << '\n';

    boost::asio::io_context ioc;

    OrderBook btc_book("BTCUSDT");

    WebsocketSession session(
        ioc,
        "stream.binance.com",
        "9443",
        "/ws/btcusdt@depth5@100ms",
        [&btc_book](std::string_view msg) {

            // Parse JSON first
            auto j = json::parse(msg);

            // Create Quote objects from JSON
            Quote bid{
                std::stod( j["bids"][0][0].get<std::string>() ),
                std::stod( j["bids"][0][1].get<std::string>() )
            };
            Quote ask{
                std::stod( j["asks"][0][0].get<std::string>() ),
                std::stod( j["asks"][0][1].get<std::string>() )
            };

            static Quote lastBid{}, lastAsk{};

            btc_book.update(j["lastUpdateId"].get<uint64_t>(), bid, ask);

            const auto bestBid = btc_book.bestBid();
            const auto bestAsk = btc_book.bestAsk();

            if(bestBid.px != lastBid.px || bestBid.qty != lastBid.qty || 
               bestAsk.px != lastAsk.px || bestAsk.qty != lastAsk.qty)
            {
                lastBid = bestBid;
                lastAsk = bestAsk;

                std::cout << std::fixed << std::setprecision(2)
                    << "Best bid: $" << bestBid.px << " (" << bestBid.qty << " BTC)\n"
                    << "Best ask: $" << bestAsk.px << " (" << bestAsk.qty << " BTC)\n"
                    << "Spread: $" << (bestAsk.px - bestBid.px)
                    << "\n\n";
                
            }
        });
            
    // Resolve DNS + Connect + Handshake (starts the async chain)
    session.run();

    // Run the I/O loop (this blocks until the program is terminated or an error occurs)
    ioc.run();

    return 0;
}
