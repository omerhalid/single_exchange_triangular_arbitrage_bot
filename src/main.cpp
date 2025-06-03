#include "common.hpp"
#include "websocket_session.hpp"
#include "orderbook.hpp"
#include <iostream>
#include <iomanip>
#include <boost/asio.hpp>
#include <unordered_map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

OrderBook btc_usdt_book("BTCUSDT");
OrderBook eth_btc_book("ETHBTC");
OrderBook eth_usdt_book("ETHUSDT");

std::atomic_bool got_btc{false}, got_ethbtc{false}, got_ethusdt{false};

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

double last_edge = 0.0;                 // remember last printed edge
constexpr double EDGE_THRESHOLD = 0.0008;

bool edgeScanner()
{

    if(!got_btc || !got_ethbtc || !got_ethusdt)
    {
        return false;
    }

    // Add validity checks
    if (eth_usdt_book.bestAsk().px <= 0 || 
        eth_btc_book.bestBid().px <= 0 || 
        btc_usdt_book.bestBid().px <= 0) {
        return false;  // Invalid prices
    }

    double maker = -0.0001;   // maker rebate
    double taker =  0.0004;   // taker fee (pay in BNB)

    auto edge =
        (1 /*USDT*/)
        /  eth_usdt_book.bestAsk().px * (1.0 + taker)     // buy ETH
        *  eth_btc_book.bestBid().px * (1.0 - maker)      // sell ETH → BTC
        *  btc_usdt_book.bestBid().px * (1.0 - maker)     // sell BTC → USDT
        - 1.0;

    if(edge > EDGE_THRESHOLD && std::abs(edge - last_edge) > 1e-6)
    {
        last_edge = edge;
        std::cout << "Edge " << edge*100 << "% -> FIRE\n";
        return true;
    }
    last_edge = edge;        // remember even if not profitable
    return false;
}

int main() {
    std::cout << "Hello, Binance! running " << version << '\n';

    boost::asio::io_context ioc;   

    std::cout << "Creating WebSocket connection...\n";  // Add debug print

    WebsocketSession session(
        ioc,
        "stream.binance.com",
        "9443",
        "/stream?streams=btcusdt@depth5@100ms/ethbtc@depth5@100ms/ethusdt@depth5@100ms",
        [&](std::string_view msg) {
            try {
                static int msg_count = 0;
                if (++msg_count % 100 == 0) {  // Log every 100th message
                    std::cout << "Processed " << msg_count << " messages\n";
                }

                auto j = json::parse(msg);
                auto stream = j["stream"].get<std::string>();
                
                // 1) Ensure it has a "stream" field:
                if (!j.contains("stream") || !j["stream"].is_string())
                    return; // not a combined‐streams data message

                // 2) Ensure "data" is an object:
                if (!j.contains("data") || !j["data"].is_object())
                    return;

                auto& data = j["data"];
                
                auto id = data["lastUpdateId"].get<uint64_t>();

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
                

                if (stream == "btcusdt@depth5@100ms") {
                    btc_usdt_book.update(id, bid, ask);
                    printBookUpdate("BTC-USDT", btc_usdt_book);
                    got_btc = true;
                }
                else if (stream == "ethbtc@depth5@100ms") {
                    eth_btc_book.update(id, bid, ask);
                    printBookUpdate("ETH-BTC", eth_btc_book);
                    got_ethbtc = true;
                }
                else if (stream == "ethusdt@depth5@100ms") {
                    eth_usdt_book.update(id, bid, ask);
                    printBookUpdate("ETH-USDT", eth_usdt_book);
                    got_ethusdt = true;
                }

                // After updating all books, scan for arbitrage opportunities
                if (edgeScanner()) {
                    std::cout << "Arbitrage opportunity found!\n";
                    // TODO: Execute trades here
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
