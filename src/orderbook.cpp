#include "orderbook.hpp"

OrderBook::OrderBook(std::string_view symbol) 
    : symbol_(std::move(symbol)) 
{}

void OrderBook::update(uint64_t updateId, const Quote& bid, const Quote& ask)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if(updateId <= lastUpdatedId_) return; // skip the old updates
    bestBid_ = bid;
    bestAsk_ = ask;
    lastUpdatedId_ = updateId;
}

Quote OrderBook::bestBid() const 
{ 
    std::lock_guard<std::mutex> lock(mutex_);
    return bestBid_;
}

Quote OrderBook::bestAsk() const 
{ 
    std::lock_guard<std::mutex> lock(mutex_);
    return bestAsk_;
}
