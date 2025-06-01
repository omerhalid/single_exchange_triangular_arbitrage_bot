#pragma once
#include "common.hpp"
#include <atomic>
#include <string>
#include <mutex>

class OrderBook
{
    public:
        explicit OrderBook(std::string_view symbol) : symbol_(std::move(symbol)) {}         

        // thread safe update from websocket
        void update(uint64_t updateId, const Quote& bid, const Quote& ask)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(updateId <= lastUpdatedId_) return; // skip the old updates
            bestBid_ = bid;
            bestAsk_ = ask;
            lastUpdatedId_ = updateId;
        }
        
        // Getters for arbitrage engine
        Quote bestBid() const 
        { 
            std::lock_guard<std::mutex> lock(mutex_);
            return bestBid_;
        }
        Quote bestAsk() const 
        { 
            std::lock_guard<std::mutex> lock(mutex_);
            return bestAsk_;
        }

    private:
        std::string symbol_;
        std::atomic<uint64_t> lastUpdatedId_{0};
        Quote bestBid_ {0.0, 0.0};
        Quote bestAsk_ {0.0, 0.0};
        mutable std::mutex mutex_;
    };