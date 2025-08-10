#pragma once
#include "OrderEvents.h"
#include "Types.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <optional>
#include "../infrastructure/LockFreeQueue.h"

// Simplified single-threaded matching engine consuming events from an SPSC queue.
// Deterministic: processes events in arrival order.
class MatchingEngine {
public:
    explicit MatchingEngine(SPSCQueue<TradingEvent>& queue);
    ~MatchingEngine();

    void start();
    void stop();

    // Access read-only snapshot of orders (for status queries) - NOT optimized.
    std::optional<Order> getOrder(const std::string& id) const;

private:
    void run();
    void onNewOrder(const NewOrderEvent& ev);
    void onCancelOrder(const CancelOrderEvent& ev);
    void onExecuteOrder(const ExecuteOrderEvent& ev);
    void onMarketData(const MarketDataUpdateEvent& ev);

    SPSCQueue<TradingEvent>& queue_;
    std::atomic<bool> running_{false};
    std::jthread thread_;
    mutable std::mutex stateMutex_;
    std::unordered_map<std::string, Order> orderBook_; // naive; future: price levels
};
