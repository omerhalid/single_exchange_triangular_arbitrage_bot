#pragma once
#include "common.hpp"
#include <atomic>
#include <string>

class OrderBook
{
    public:
        explicit OrderBook(std::string_view symbol) : symbol_(std::move(symbol)) {}         

        // thread safe update from websocket
        void update(uint64_t updateId, const Quote& bid, const Quote& ask)
        {
            if(updateId <= lastUpdatedId_) return; // skip the old updates
            bestBid_ = bid;
            bestAsk_ = ask;
            lastUpdatedId_ = updateId;
        }
        
        // Getters for arbitrage engine
        Quote bestBid() const { return bestBid_;}
        Quote bestAsk() const { return bestAsk_;}

    private:
        std::string_view symbol_;
        std::atomic<uint64_t> lastUpdatedId_{0};
        Quote bestBid_ {0.0, 0.0};
        Quote bestAsk_ {0.0, 0.0};
    };