#include "common.hpp"
#include "websocket_session.hpp"
#include <iostream>
#include <boost/asio.hpp>    // For boost::asio::io_context

int main() {
    std::cout << "Hello, Binance! running " << version << '\n';

    boost::asio::io_context ioc;

    WebsocketSession session(
        ioc,
        "testnet.binance.vision",   // host
        "443",                      // port (TLS)
        "/ws/btcusdt@depth",        // path that the test-net serves
        [](std::string_view f){ std::cout << f << '\n'; });


    // Resolve DNS + Connect + Handshake (starts the async chain)
    session.run();

    // Run the I/O loop (this blocks until the program is terminated or an error occurs)
    ioc.run();

    return 0;
}
