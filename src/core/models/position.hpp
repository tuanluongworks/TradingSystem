#pragma once

#include <string>
#include <chrono>
#include <mutex>

namespace trading {

class Position {
public:
    // Constructor
    Position(const std::string& instrument_symbol);

    // Getters
    const std::string& get_instrument_symbol() const { return instrument_symbol_; }
    double get_quantity() const;
    double get_average_price() const;
    double get_realized_pnl() const;
    double get_unrealized_pnl() const;
    std::chrono::system_clock::time_point get_last_updated() const;

    // Calculated fields
    double get_market_value(double current_price) const;
    double get_total_pnl(double current_price) const;
    bool is_flat() const;           // quantity == 0
    bool is_long() const;           // quantity > 0
    bool is_short() const;          // quantity < 0

    // Position updates (thread-safe)
    void add_trade(double quantity, double price);
    void update_unrealized_pnl(double current_price);
    void close_position();

    // Validation
    bool is_valid() const;

private:
    const std::string instrument_symbol_; // Reference to Instrument

    // Mutable state (protected by mutex)
    mutable std::mutex position_mutex_;
    double quantity_;                      // Net position (positive=long, negative=short)
    double average_price_;                 // Volume-weighted average price
    double realized_pnl_;                  // Profit/loss from closed trades
    double unrealized_pnl_;                // Current mark-to-market P&L
    std::chrono::system_clock::time_point last_updated_;

    // Helper methods
    void update_last_modified();
    void recalculate_average_price(double new_quantity, double new_price);
    double calculate_realized_pnl(double closing_quantity, double closing_price);
};

} // namespace trading