#pragma once
#include "OrderEvents.h"
#include "Types.h"
#include <unordered_map>
#include <map>
#include <deque>
#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <optional>
#include "../infrastructure/LockFreeQueue.h"

struct OrderEntry { Order order; };
struct PriceLevel { std::deque<OrderEntry> fifo; };

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
    void onTradeExecution(const TradeExecutionEvent& ev); // pass-through if needed
    void match(Order& incoming);
    void addToBook(const Order& o);

    SPSCQueue<TradingEvent>& queue_;
    std::atomic<bool> running_{false};
    std::jthread thread_;
    mutable std::mutex stateMutex_;
    std::unordered_map<std::string, Order> orderIndex_;
    // Price-time priority books: bids descending, asks ascending
    std::map<double, PriceLevel, std::greater<double>> bids_; // buy orders
    std::map<double, PriceLevel, std::less<double>> asks_;    // sell orders
};
