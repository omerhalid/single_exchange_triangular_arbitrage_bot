#include "triarb_bot.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <iomanip>
#include <unordered_map>

using json = nlohmann::json;

namespace triarb {

bool load_live_toggle_from_env()
{
    const char* live = std::getenv("LIVE");
    if (!live) {
        throw std::runtime_error("Missing LIVE toggle in environment");
    }
    
    // Convert string to bool
    std::string live_str(live);
    return live_str == "true" || live_str == "1";
}

ApiKeys load_keys_from_env() 
{
    const char* api_key = std::getenv("BINANCE_API_KEY");
    const char* api_secret = std::getenv("BINANCE_API_SECRET");
    
    if (!api_key || !api_secret) {
        throw std::runtime_error("Missing BINANCE_API_KEY or BINANCE_API_SECRET in environment");
    }
    
    return ApiKeys{api_key, api_secret};
}

TriArbBot::TriArbBot(boost::asio::io_context& ioc)
    : gw_(ioc, 
         "api.binance.com", 
         load_keys_from_env(),
         load_live_toggle_from_env())
    , session_(ioc,
              "stream.binance.com",
              "9443",
              "/stream?streams=btcusdt@depth5@100ms/ethbtc@depth5@100ms/ethusdt@depth5@100ms",
              [this](std::string_view msg) { handle_frame(msg); })
{
    // Check MAX_NOTIONAL
    if (!std::getenv("MAX_NOTIONAL")) {
        std::cerr << "Warning: MAX_NOTIONAL unset—defaulting to 15 USDT\n";
    }
}

void TriArbBot::start() { session_.run(); }

void TriArbBot::print_book_update(const std::string& symbol, const OrderBook& book)
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
        auto precision = (symbol == "ETH-BTC") ? 8 : 2;
        std::cout << std::fixed << std::setprecision(precision)
                  << symbol << " - "
                  << "Best bid: " << bestBid.px << " (" << bestBid.qty << ")\n"
                  << "Best ask: " << bestAsk.px << " (" << bestAsk.qty << ")\n"
                  << "Spread: " << (bestAsk.px - bestBid.px) << "\n\n";
    }
}

bool TriArbBot::edge_scanner()
{
    if(!got_btc_ || !got_ethbtc_ || !got_ethusdt_)
        return false;

    if (eth_usdt_book_.bestAsk().px <= 0 ||
        eth_btc_book_.bestBid().px <= 0 ||
        btc_usdt_book_.bestBid().px <= 0)
        return false;

    double maker = -0.0001;
    double taker =  0.0004;

    auto edge =
        (1.0)
        /  eth_usdt_book_.bestAsk().px * (1.0 + taker)
        *  eth_btc_book_.bestBid().px * (1.0 - maker)
        *  btc_usdt_book_.bestBid().px * (1.0 - maker)
        - 1.0;

    constexpr double EDGE_THRESHOLD = 0.0008;
    if(edge > EDGE_THRESHOLD && std::abs(edge - last_edge_) > 1e-6)
    {
        last_edge_ = edge;
        std::cout << "Edge " << edge*100 << "% -> FIRE\n";
        return true;
    }
    last_edge_ = edge;
    return false;
}

void TriArbBot::handle_frame(std::string_view msg)
{
    try {
        static int msg_count = 0;
        if (++msg_count % 100 == 0)
            std::cout << "Processed " << msg_count << " messages\n";

        auto j = json::parse(msg);

        if (!j.contains("stream") || !j["stream"].is_string())
            return;
        if (!j.contains("data") || !j["data"].is_object())
            return;
        auto& data = j["data"];
        auto stream = j["stream"].get<std::string>();
        auto id = data["lastUpdateId"].get<uint64_t>();
        if (!data.contains("bids") || !data["bids"].is_array() || data["bids"].empty())
            return;
        if (!data.contains("asks") || !data["asks"].is_array() || data["asks"].empty())
            return;

        Quote bid {
            std::stod(data["bids"][0][0].get<std::string>()),
            std::stod(data["bids"][0][1].get<std::string>())
        };
        Quote ask {
            std::stod(data["asks"][0][0].get<std::string>()),
            std::stod(data["asks"][0][1].get<std::string>())
        };

        if (stream == "btcusdt@depth5@100ms") {
            btc_usdt_book_.update(id, bid, ask);
            print_book_update("BTC-USDT", btc_usdt_book_);
            got_btc_ = true;
        } else if (stream == "ethbtc@depth5@100ms") {
            eth_btc_book_.update(id, bid, ask);
            print_book_update("ETH-BTC", eth_btc_book_);
            got_ethbtc_ = true;
        } else if (stream == "ethusdt@depth5@100ms") {
            eth_usdt_book_.update(id, bid, ask);
            print_book_update("ETH-USDT", eth_usdt_book_);
            got_ethusdt_ = true;
        }

        if (edge_scanner()) {
            std::cout << "Arbitrage opportunity found!\n";

            // Load triangle size from environment
            const char* env_qty = std::getenv("MAX_NOTIONAL");
            double max_notional = env_qty ? std::stod(env_qty) : 15.0;
            double qty = max_notional;  // USDT to spend on ETHUSDT

            double eth_usdt_ask = eth_usdt_book_.bestAsk().px;
            gw_.send_order("ETHUSDT", "BUY",
                qty/eth_usdt_ask,
                eth_usdt_ask,
                [&](FillReport rep) {
                    if (!rep.success) return;
                    std::cout << "[GATE] Step 1: Bought ETH with USDT: " 
                             << rep.qty_filled << " ETH @ " << rep.price_avg << " USDT\n";

                    // Step 2: Sell ETH for BTC
                    double eth_btc_bid = eth_btc_book_.bestBid().px;
                    gw_.send_order("ETHBTC", "SELL", 
                        rep.qty_filled,
                        eth_btc_bid,
                        [&](FillReport rep2) {
                            if (!rep2.success) return;
                            std::cout << "[GATE] Step 2: Sold ETH for BTC: " 
                                     << rep2.qty_filled << " ETH @ " << rep2.price_avg << " BTC\n";

                            // Step 3: Sell BTC for USDT
                            double btc_amount = rep2.qty_filled * rep2.price_avg;
                            double btc_usdt_bid = btc_usdt_book_.bestBid().px;
                            gw_.send_order("BTCUSDT", "SELL",    
                                btc_amount,
                                btc_usdt_bid,
                                [&](FillReport rep3) {
                                    std::cout << "[GATE] Step 3: Sold BTC for USDT: "
                                             << rep3.qty_filled << " BTC @ " << rep3.price_avg << " USDT\n";
                                });
                        });
                });
        }


    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

} // namespace triarb


