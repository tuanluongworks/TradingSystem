#include "MatchingEngine.h"
#include <iostream>
#include <chrono>
#include <mutex>

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
    auto it = orderIndex_.find(id);
    if (it!=orderIndex_.end()) return it->second;
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
            else if constexpr (std::is_same_v<E, TradeExecutionEvent>) onTradeExecution(e);
            else if constexpr (std::is_same_v<E, ShutdownEvent>) running_ = false;
        }, evt);
    }
}

void MatchingEngine::onNewOrder(const NewOrderEvent& ev) {
    Order copy = ev.order;
    std::scoped_lock lock(stateMutex_);
    orderIndex_.emplace(copy.id, copy);
    match(orderIndex_[copy.id]);
}

void MatchingEngine::onCancelOrder(const CancelOrderEvent& ev) {
    std::scoped_lock lock(stateMutex_);
    auto it = orderIndex_.find(ev.orderId);
    if (it!=orderIndex_.end()) {
        it->second.status = OrderStatus::CANCELLED;
    }
}

void MatchingEngine::onExecuteOrder(const ExecuteOrderEvent& ev) {
    std::scoped_lock lock(stateMutex_);
    auto it = orderIndex_.find(ev.orderId);
    if (it!=orderIndex_.end()) {
        it->second.status = OrderStatus::FILLED;
    }
}

void MatchingEngine::onMarketData(const MarketDataUpdateEvent& ev) {
    (void)ev;
}

void MatchingEngine::onTradeExecution(const TradeExecutionEvent& ev) {
    (void)ev;
}

void MatchingEngine::addToBook(const Order& o) {
    if (o.type==OrderType::BUY) {
        auto &lvl = bids_[o.price];
        lvl.fifo.push_back({o});
    } else {
        auto &lvl = asks_[o.price];
        lvl.fifo.push_back({o});
    }
}

void MatchingEngine::match(Order& incoming) {
    if (incoming.type==OrderType::BUY) {
        // Try to match against lowest asks (asks_ ascending)
        for (auto it = asks_.begin(); it != asks_.end() && incoming.quantity>0 && incoming.price >= it->first; ) {
            auto &fifo = it->second.fifo;
            while(!fifo.empty() && incoming.quantity>0) {
                auto &resting = fifo.front().order;
                double execQty = std::min(incoming.quantity, resting.quantity);
                double execPrice = it->first;
                incoming.quantity -= execQty;
                resting.quantity -= execQty;

                // Emit execution events
                queue_.push(TradingEvent{TradeExecutionEvent{resting, execPrice, execQty}});
                queue_.push(TradingEvent{TradeExecutionEvent{incoming, execPrice, execQty}});

                if (resting.quantity==0) {
                    orderIndex_[resting.id].status = OrderStatus::FILLED;
                    fifo.pop_front();
                } else {
                    orderIndex_[resting.id].quantity = resting.quantity;
                }
            }
            if (fifo.empty()) it = asks_.erase(it); else ++it;
        }
        if (incoming.quantity>0) {
            addToBook(incoming);
        } else {
            orderIndex_[incoming.id].status = OrderStatus::FILLED;
        }
    } else { // SELL
        for (auto it = bids_.begin(); it != bids_.end() && incoming.quantity>0 && incoming.price <= it->first; ) {
            auto &fifo = it->second.fifo;
            while(!fifo.empty() && incoming.quantity>0) {
                auto &resting = fifo.front().order;
                double execQty = std::min(incoming.quantity, resting.quantity);
                double execPrice = it->first;
                incoming.quantity -= execQty;
                resting.quantity -= execQty;

                queue_.push(TradingEvent{TradeExecutionEvent{resting, execPrice, execQty}});
                queue_.push(TradingEvent{TradeExecutionEvent{incoming, execPrice, execQty}});

                if (resting.quantity==0) {
                    orderIndex_[resting.id].status = OrderStatus::FILLED;
                    fifo.pop_front();
                } else {
                    orderIndex_[resting.id].quantity = resting.quantity;
                }
            }
            if (fifo.empty()) it = bids_.erase(it); else ++it;
        }
        if (incoming.quantity>0) {
            addToBook(incoming);
        } else {
            orderIndex_[incoming.id].status = OrderStatus::FILLED;
        }
    }
}
