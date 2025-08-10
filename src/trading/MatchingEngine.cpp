#include "MatchingEngine.h"
#include <iostream>
#include <chrono>

MatchingEngine::MatchingEngine(SPSCQueue<TradingEvent>& queue) : queue_(queue) {}
MatchingEngine::~MatchingEngine() { stop(); }

void MatchingEngine::start() {
    if (running_.exchange(true)) return;
    thread_ = std::jthread([this]{ run(); });
}

void MatchingEngine::stop() {
    if (!running_.exchange(false)) return;
    // push shutdown signal
    queue_.push(TradingEvent{ShutdownEvent{}});
    if (thread_.joinable()) thread_.join();
}

std::optional<Order> MatchingEngine::getOrder(const std::string& id) const {
    std::scoped_lock lock(stateMutex_);
    auto it = orderBook_.find(id);
    if (it != orderBook_.end()) return it->second;
    return std::nullopt;
}

void MatchingEngine::run() {
    while (running_) {
        auto evtOpt = queue_.pop();
        if (!evtOpt) {
            std::this_thread::sleep_for(std::chrono::microseconds(50));
            continue;
        }
        auto &evt = *evtOpt;
        std::visit([this](auto&& e){
            using E = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<E, NewOrderEvent>) onNewOrder(e);
            else if constexpr (std::is_same_v<E, CancelOrderEvent>) onCancelOrder(e);
            else if constexpr (std::is_same_v<E, ExecuteOrderEvent>) onExecuteOrder(e);
            else if constexpr (std::is_same_v<E, MarketDataUpdateEvent>) onMarketData(e);
            else if constexpr (std::is_same_v<E, ShutdownEvent>) running_ = false;
        }, evt);
    }
}

void MatchingEngine::onNewOrder(const NewOrderEvent& ev) {
    std::scoped_lock lock(stateMutex_);
    orderBook_.emplace(ev.order.id, ev.order);
    // future: match logic
}

void MatchingEngine::onCancelOrder(const CancelOrderEvent& ev) {
    std::scoped_lock lock(stateMutex_);
    auto it = orderBook_.find(ev.orderId);
    if (it != orderBook_.end()) {
        it->second.status = OrderStatus::CANCELLED;
    }
}

void MatchingEngine::onExecuteOrder(const ExecuteOrderEvent& ev) {
    std::scoped_lock lock(stateMutex_);
    auto it = orderBook_.find(ev.orderId);
    if (it != orderBook_.end()) {
        it->second.status = OrderStatus::FILLED;
    }
}

void MatchingEngine::onMarketData(const MarketDataUpdateEvent& ev) {
    // placeholder: could update internal price levels or trigger matches
    (void)ev; // suppress unused warning
}
