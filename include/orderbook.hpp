#pragma once
#include "common.hpp"
#include <atomic>
#include <string>
#include <mutex>

namespace triarb {

class OrderBook
{
    public:
        explicit OrderBook(std::string_view symbol);
        void update(uint64_t updateId, const Quote& bid, const Quote& ask);
        Quote bestBid() const;
        Quote bestAsk() const;

    private:
        std::string symbol_;
        std::atomic<uint64_t> lastUpdatedId_{0};
        Quote bestBid_ {0.0, 0.0};
        Quote bestAsk_ {0.0, 0.0};
        mutable std::mutex mutex_;
};

} // namespace triarb
