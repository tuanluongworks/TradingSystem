#pragma once

#include <string>

namespace trading_core {

struct Tick {
    std::string symbol;
    double bid_price;
    double ask_price;
    double last_price;
    int timestamp;
};

enum class OrderSide {
    BUY,
    SELL
};

enum class OrderType {
    MARKET,
    LIMIT
};

struct Order {
    long order_id;
    std::string symbol;
    OrderSide side;
    OrderType type;
    double price;
    int quantity;
};

enum class ExecutionStatus {
    FILLED,
    PARTIALLY_FILLED,
    CANCELED
};

struct ExecutionReport {
    long order_id;
    std::string symbol;
    ExecutionStatus status;
    double price;
    int quantity;
};

} // namespace trading_core