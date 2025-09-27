#pragma once

#include <string>
#include <chrono>
#include <mutex>

namespace trading {

enum class InstrumentType {
    STOCK,
    FOREX,
    CRYPTO,
    COMMODITY,
    INDEX
};

class Instrument {
public:
    // Constructor
    Instrument(const std::string& symbol, const std::string& name,
               InstrumentType type, double tick_size, int lot_size);

    // Getters
    const std::string& get_symbol() const { return symbol_; }
    const std::string& get_name() const { return name_; }
    InstrumentType get_type() const { return type_; }
    double get_tick_size() const { return tick_size_; }
    int get_lot_size() const { return lot_size_; }
    bool is_active() const { return is_active_; }

    // Market data (mutable, updated real-time)
    double get_bid_price() const { return bid_price_; }
    double get_ask_price() const { return ask_price_; }
    double get_last_price() const { return last_price_; }
    std::chrono::system_clock::time_point get_last_update() const { return last_update_; }

    // Setters for configuration
    void set_active(bool active) { is_active_ = active; }

    // Market data updates (thread-safe)
    void update_market_data(double bid, double ask, double last);

    // Validation
    bool is_valid() const;
    bool is_price_valid(double price) const;
    bool is_quantity_valid(int quantity) const;

    // Utility functions
    double round_to_tick_size(double price) const;
    int round_to_lot_size(int quantity) const;
    double get_spread() const;
    double get_mid_price() const;

private:
    // Static configuration
    std::string symbol_;           // Unique identifier (e.g., "AAPL", "EURUSD")
    std::string name_;             // Human-readable name
    InstrumentType type_;          // STOCK, FOREX, CRYPTO, etc.
    double tick_size_;             // Minimum price increment
    int lot_size_;                 // Minimum quantity increment
    bool is_active_;               // Trading enabled flag

    // Market data (mutable, updated real-time)
    mutable double bid_price_;     // Current bid price
    mutable double ask_price_;     // Current ask price
    mutable double last_price_;    // Last traded price
    mutable std::chrono::system_clock::time_point last_update_;

    // Thread safety for market data updates
    mutable std::mutex market_data_mutex_;
};

// Helper functions
std::string instrument_type_to_string(InstrumentType type);
InstrumentType string_to_instrument_type(const std::string& type_str);

} // namespace trading