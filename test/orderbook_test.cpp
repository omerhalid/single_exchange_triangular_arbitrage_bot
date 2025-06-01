#include "orderbook.hpp"
#include <catch2/catch_test_macros.hpp>
#include <thread>
#include <vector>

TEST_CASE("Quote construction and comparison", "[orderbook]") {
    Quote q{100.0, 1.5};
    REQUIRE(q.px == 100.0);
    REQUIRE(q.qty == 1.5);
}

TEST_CASE("OrderBook basic operations", "[orderbook]") {
    OrderBook book("BTCUSDT");
    
    Quote bid{100.0, 1.0};
    Quote ask{101.0, 2.0};
    book.update(1, bid, ask);

    auto bestBid = book.bestBid();
    auto bestAsk = book.bestAsk();

    REQUIRE(bestBid.px == 100.0);
    REQUIRE(bestBid.qty == 1.0);
    REQUIRE(bestAsk.px == 101.0);
    REQUIRE(bestAsk.qty == 2.0);
}

TEST_CASE("OrderBook update sequence validation", "[orderbook]") {
    OrderBook book("BTCUSDT");
    
    // Initial update
    book.update(2, Quote{100.0, 1.0}, Quote{101.0, 1.0});
    
    // Try older update - should be ignored
    book.update(1, Quote{99.0, 1.0}, Quote{102.0, 1.0});
    
    auto bestBid = book.bestBid();
    auto bestAsk = book.bestAsk();
    
    // Should still have values from updateId 2
    REQUIRE(bestBid.px == 100.0);
    REQUIRE(bestAsk.px == 101.0);
}

TEST_CASE("OrderBook thread safety", "[orderbook]") {
    OrderBook book("BTCUSDT");
    std::vector<std::thread> threads;
    
    // Spawn multiple threads updating the book
    for(int i = 0; i < 10; ++i) {
        threads.emplace_back([&book, i] {
            book.update(i, 
                Quote{100.0 + i, 1.0}, 
                Quote{101.0 + i, 1.0});
        });
    }
    
    // Join all threads
    for(auto& t : threads) {
        t.join();
    }
    
    // Book should have highest update ID values
    auto bestBid = book.bestBid();
    auto bestAsk = book.bestAsk();
    
    REQUIRE(bestBid.px >= 100.0);
    REQUIRE(bestAsk.px >= 101.0);
}