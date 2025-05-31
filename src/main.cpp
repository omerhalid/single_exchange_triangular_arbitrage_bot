#include "common.hpp"
#include "websocket_session.hpp"
#include <iostream>
#include <boost/asio.hpp>    // For boost::asio::io_context

int main() {
    std::cout << "Hello, Binance! running " << version << '\n';

    boost::asio::io_context ioc;

    WebsocketSession session(
        ioc,
        "stream.binance.com",      // host  (main-net)
        "9443",                    // port  (Binance WS TLS port)
        "/ws/btcusdt@depth5@100ms",// path  (any of depth, depth5, aggTrade …)
        [](std::string_view json){ std::cout << json << '\n'; });

    // Resolve DNS + Connect + Handshake (starts the async chain)
    session.run();

    // Run the I/O loop (this blocks until the program is terminated or an error occurs)
    ioc.run();

    return 0;
}
