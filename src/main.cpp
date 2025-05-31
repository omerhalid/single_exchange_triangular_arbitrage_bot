#include "common.hpp"
#include "websocket_session.hpp"
#include <iostream>
#include <boost/asio.hpp>    // For boost::asio::io_context
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
    std::cout << "Hello, Binance! running " << version << '\n';

    boost::asio::io_context ioc;

    WebsocketSession session(
        ioc,
        "stream.binance.com",
        "9443",
        "/ws/btcusdt@depth5@100ms",
        [](std::string_view msg) {
            auto j = json::parse(msg);
            std::cout << "Best bid: " << j["bids"][0][0] << " (" << j["bids"][0][1] << " BTC)\n"
                     << "Best ask: " << j["asks"][0][0] << " (" << j["asks"][0][1] << " BTC)\n"
                     << "Spread: $" << (std::stod(std::string(j["asks"][0][0])) - 
                                      std::stod(std::string(j["bids"][0][0])))
                     << "\n\n";
        });

    // Resolve DNS + Connect + Handshake (starts the async chain)
    session.run();

    // Run the I/O loop (this blocks until the program is terminated or an error occurs)
    ioc.run();

    return 0;
}
