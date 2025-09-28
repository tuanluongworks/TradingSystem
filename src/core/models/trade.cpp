#include "trade.hpp"
#include <stdexcept>
#include <algorithm>

namespace trading {

Trade::Trade(const std::string& trade_id, const std::string& order_id,
             const std::string& instrument_symbol, OrderSide side,
             double quantity, double price, TradeType type)
    : trade_id_(trade_id), order_id_(order_id), instrument_symbol_(instrument_symbol),
      side_(side), quantity_(quantity), price_(price),
      execution_time_(std::chrono::system_clock::now()), type_(type) {

    if (trade_id.empty()) {
        throw std::invalid_argument("Trade ID cannot be empty");
    }
    if (order_id.empty()) {
        throw std::invalid_argument("Order ID cannot be empty");
    }
    if (instrument_symbol.empty()) {
        throw std::invalid_argument("Instrument symbol cannot be empty");
    }
    if (quantity <= 0) {
        throw std::invalid_argument("Quantity must be positive");
    }
    if (price <= 0) {
        throw std::invalid_argument("Price must be positive");
    }
}

double Trade::get_notional_value() const {
    return quantity_ * price_;
}

double Trade::get_commission() const {
    double commission = get_notional_value() * COMMISSION_RATE;
    return std::max(commission, MIN_COMMISSION);
}

double Trade::get_net_value() const {
    return get_notional_value() - get_commission();
}

bool Trade::is_valid() const {
    return !trade_id_.empty() &&
           !order_id_.empty() &&
           !instrument_symbol_.empty() &&
           quantity_ > 0 &&
           price_ > 0;
}

bool Trade::operator<(const Trade& other) const {
    // Sort by execution time (most recent first)
    return execution_time_ > other.execution_time_;
}

bool Trade::operator==(const Trade& other) const {
    return trade_id_ == other.trade_id_;
}

// Helper functions
std::string trade_type_to_string(TradeType type) {
    switch (type) {
        case TradeType::FULL_FILL: return "FULL_FILL";
        case TradeType::PARTIAL_FILL: return "PARTIAL_FILL";
        default: return "UNKNOWN";
    }
}

TradeType string_to_trade_type(const std::string& type_str) {
    if (type_str == "FULL_FILL") return TradeType::FULL_FILL;
    if (type_str == "PARTIAL_FILL") return TradeType::PARTIAL_FILL;
    throw std::invalid_argument("Unknown trade type: " + type_str);
}

} // namespace trading