#pragma once

#include "contracts/trading_engine_api.hpp"
#include <string>
#include <chrono>
#include <mutex>

namespace trading {

class Order {
public:
    // Constructor
    Order(const std::string& order_id, const std::string& instrument_symbol,
          OrderSide side, OrderType type, double quantity, double price = 0.0);

    // Getters
    const std::string& get_order_id() const { return order_id_; }
    const std::string& get_instrument_symbol() const { return instrument_symbol_; }
    OrderSide get_side() const { return side_; }
    OrderType get_type() const { return type_; }
    double get_quantity() const { return quantity_; }
    double get_price() const { return price_; }
    OrderStatus get_status() const;
    double get_filled_quantity() const;
    double get_remaining_quantity() const;
    std::chrono::system_clock::time_point get_created_time() const { return created_time_; }
    std::chrono::system_clock::time_point get_last_modified() const;
    const std::string& get_rejection_reason() const;

    // Calculated fields
    double get_average_fill_price() const;
    bool is_fully_filled() const;
    bool is_working() const;
    bool is_cancelable() const;

    // State management (thread-safe)
    bool accept();
    bool reject(const std::string& reason);
    bool cancel();
    bool fill(double quantity, double price);
    bool partial_fill(double quantity, double price);

    // Validation
    bool is_valid() const;
    bool is_status_transition_valid(OrderStatus new_status) const;

private:
    // Immutable order data
    const std::string order_id_;         // Unique system-generated ID
    const std::string instrument_symbol_; // Reference to Instrument
    const OrderSide side_;               // BUY or SELL
    const OrderType type_;               // MARKET or LIMIT
    const double quantity_;              // Requested quantity
    const double price_;                 // Limit price (0 for market orders)
    const std::chrono::system_clock::time_point created_time_;

    // Mutable state (protected by mutex)
    mutable std::mutex state_mutex_;
    OrderStatus status_;                 // Current order state
    double filled_quantity_;             // Quantity already executed
    double total_fill_value_;            // Total value of all fills (for avg price calc)
    std::chrono::system_clock::time_point last_modified_;
    std::string rejection_reason_;       // If status == REJECTED

    // Helper methods
    void update_last_modified();
    bool is_terminal_status(OrderStatus status) const;
};

// Helper functions
std::string order_side_to_string(OrderSide side);
std::string order_type_to_string(OrderType type);
std::string order_status_to_string(OrderStatus status);

OrderSide string_to_order_side(const std::string& side_str);
OrderType string_to_order_type(const std::string& type_str);
OrderStatus string_to_order_status(const std::string& status_str);

} // namespace trading