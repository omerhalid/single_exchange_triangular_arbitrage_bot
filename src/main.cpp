#include "common.hpp"
#include "websocket_session.hpp"
#include <boost/asio.hpp>
#include <iostream>

int main(){
    std::cout<<"Hello, Binance! running "<<version<<'\n';

    boost::asio:io_context ioc;

    WebsocketSession session(
        ioc,
        "testnet.binance.vision",   // host
        "443",                      // port
        "/stream?streams=btcusdt@depth5@100ms",
        [](std::string_view frame){
            std::cout << frame << '\n';      // later we'll push to queue
        });

    session.run();
    ioc.run();

    return 0;
}
 