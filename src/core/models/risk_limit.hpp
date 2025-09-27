#pragma once

#include "contracts/trading_engine_api.hpp"
#include <string>

namespace trading {

// Forward declarations
class Order;
class Position;

enum class LimitType {
    MAX_POSITION_SIZE,    // Maximum net position quantity
    MAX_ORDER_SIZE,       // Maximum single order quantity
    MAX_DAILY_VOLUME,     // Maximum daily trading volume
    MAX_LOSS_LIMIT        // Maximum daily loss limit
};

class RiskLimit {
public:
    // Constructor
    RiskLimit(const std::string& instrument_symbol, LimitType type, double max_value);
    RiskLimit(LimitType type, double max_value); // Global limit constructor

    // Getters
    const std::string& get_instrument_symbol() const { return instrument_symbol_; }
    LimitType get_type() const { return type_; }
    double get_max_value() const { return max_value_; }
    bool is_active() const { return is_active_; }
    bool is_global() const { return instrument_symbol_.empty(); }

    // Setters
    void set_max_value(double max_value);
    void set_active(bool active) { is_active_ = active; }

    // Validation methods
    bool check_order(const OrderRequest& request) const;
    bool check_order_with_position(const OrderRequest& request, const Position* current_position) const;
    bool check_position_limit(double current_quantity, double order_quantity) const;
    bool check_order_size_limit(double order_quantity) const;
    bool check_daily_volume_limit(double current_daily_volume, double order_quantity) const;
    bool check_daily_loss_limit(double current_daily_pnl) const;

    // Get violation reason
    std::string get_violation_reason(const OrderRequest& request) const;
    std::string get_violation_reason_with_position(const OrderRequest& request, const Position* current_position) const;

    // Validation
    bool is_valid() const;

    // Comparison operators
    bool operator==(const RiskLimit& other) const;
    bool operator<(const RiskLimit& other) const;

private:
    std::string instrument_symbol_; // Reference to Instrument (empty = global)
    LimitType type_;               // POSITION_LIMIT, ORDER_SIZE_LIMIT, etc.
    double max_value_;             // Maximum allowed value
    bool is_active_;               // Enable/disable limit

    // Helper methods
    std::string get_limit_description() const;
};

// Helper functions
std::string limit_type_to_string(LimitType type);
LimitType string_to_limit_type(const std::string& type_str);

} // namespace trading