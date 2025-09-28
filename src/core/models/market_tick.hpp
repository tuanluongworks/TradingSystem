#pragma once

#include <string>
#include <chrono>

namespace trading {

struct MarketTick {
    std::string instrument_symbol;
    double bid_price;
    double ask_price;
    double last_price;
    double volume;                // Trade volume
    std::chrono::system_clock::time_point timestamp;

    // Constructors
    MarketTick() = default;
    MarketTick(const std::string& symbol, double bid, double ask, double last, double vol);

    // Validation
    bool is_valid() const;
    bool is_stale(std::chrono::milliseconds threshold = std::chrono::milliseconds(5000)) const;

    // Utility functions
    double get_spread() const;           // ask - bid
    double get_mid_price() const;        // (bid + ask) / 2
    double get_spread_percent() const;   // spread / mid_price * 100

    // Comparison operators
    bool operator<(const MarketTick& other) const;  // Sort by timestamp
    bool operator==(const MarketTick& other) const;

    // String formatting
    std::string to_string() const;
    std::string get_formatted_timestamp() const;
};

} // namespace trading