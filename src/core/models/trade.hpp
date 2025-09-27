#pragma once

#include "contracts/trading_engine_api.hpp"
#include <string>
#include <chrono>

namespace trading {

enum class TradeType {
    FULL_FILL,    // Order completely executed
    PARTIAL_FILL  // Order partially executed
};

class Trade {
public:
    // Constructor
    Trade(const std::string& trade_id, const std::string& order_id,
          const std::string& instrument_symbol, OrderSide side,
          double quantity, double price, TradeType type = TradeType::FULL_FILL);

    // Getters
    const std::string& get_trade_id() const { return trade_id_; }
    const std::string& get_order_id() const { return order_id_; }
    const std::string& get_instrument_symbol() const { return instrument_symbol_; }
    OrderSide get_side() const { return side_; }
    double get_quantity() const { return quantity_; }
    double get_price() const { return price_; }
    std::chrono::system_clock::time_point get_execution_time() const { return execution_time_; }
    TradeType get_type() const { return type_; }

    // Calculated fields
    double get_notional_value() const;      // quantity * price
    double get_commission() const;          // Based on trade size
    double get_net_value() const;           // notional - commission

    // Validation
    bool is_valid() const;

    // Comparison operators (for sorting)
    bool operator<(const Trade& other) const;
    bool operator==(const Trade& other) const;

private:
    const std::string trade_id_;         // Unique system-generated ID
    const std::string order_id_;         // Reference to parent Order
    const std::string instrument_symbol_; // Reference to Instrument
    const OrderSide side_;               // BUY or SELL (copied from Order)
    const double quantity_;              // Executed quantity
    const double price_;                 // Execution price
    const std::chrono::system_clock::time_point execution_time_;
    const TradeType type_;               // FULL or PARTIAL fill

    // Commission calculation (simple fixed rate for now)
    static constexpr double COMMISSION_RATE = 0.001; // 0.1%
    static constexpr double MIN_COMMISSION = 1.0;    // Minimum $1 commission
};

// Helper functions
std::string trade_type_to_string(TradeType type);
TradeType string_to_trade_type(const std::string& type_str);

} // namespace trading