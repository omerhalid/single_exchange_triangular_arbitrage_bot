#include "common.hpp"
#include "triarb_bot.hpp"
#include <boost/asio.hpp>
#include <iostream>

int main() {
    std::cout << "Hello, Binance! running " << triarb::version << '\n';
    boost::asio::io_context ioc;
    triarb::TriArbBot bot{ioc};
    bot.start();
    ioc.run();
    return 0;
}

