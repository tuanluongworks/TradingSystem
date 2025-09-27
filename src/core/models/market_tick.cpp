#include "market_tick.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace trading {

MarketTick::MarketTick(const std::string& symbol, double bid, double ask, double last, double vol)
    : instrument_symbol(symbol), bid_price(bid), ask_price(ask), last_price(last),
      volume(vol), timestamp(std::chrono::system_clock::now()) {
}

bool MarketTick::is_valid() const {
    // Basic validation
    if (instrument_symbol.empty()) return false;
    if (bid_price < 0 || ask_price < 0 || last_price < 0) return false;
    if (volume < 0) return false;

    // Bid/ask spread validation
    if (bid_price > 0 && ask_price > 0 && ask_price < bid_price) return false;

    // Timestamp validation (not too far in the future)
    auto now = std::chrono::system_clock::now();
    auto future_threshold = now + std::chrono::minutes(1);
    if (timestamp > future_threshold) return false;

    return true;
}

bool MarketTick::is_stale(std::chrono::milliseconds threshold) const {
    auto now = std::chrono::system_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - timestamp);
    return age > threshold;
}

double MarketTick::get_spread() const {
    if (ask_price > 0 && bid_price > 0) {
        return ask_price - bid_price;
    }
    return 0.0;
}

double MarketTick::get_mid_price() const {
    if (ask_price > 0 && bid_price > 0) {
        return (ask_price + bid_price) / 2.0;
    }
    return last_price;
}

double MarketTick::get_spread_percent() const {
    double mid = get_mid_price();
    if (mid > 0) {
        return (get_spread() / mid) * 100.0;
    }
    return 0.0;
}

bool MarketTick::operator<(const MarketTick& other) const {
    // Sort by timestamp (most recent first)
    return timestamp > other.timestamp;
}

bool MarketTick::operator==(const MarketTick& other) const {
    return instrument_symbol == other.instrument_symbol &&
           std::abs(bid_price - other.bid_price) < 1e-8 &&
           std::abs(ask_price - other.ask_price) < 1e-8 &&
           std::abs(last_price - other.last_price) < 1e-8 &&
           std::abs(volume - other.volume) < 1e-8 &&
           timestamp == other.timestamp;
}

std::string MarketTick::to_string() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << instrument_symbol << ": ";
    oss << "Bid=" << bid_price << ", ";
    oss << "Ask=" << ask_price << ", ";
    oss << "Last=" << last_price << ", ";
    oss << "Volume=" << volume << ", ";
    oss << "Time=" << get_formatted_timestamp();
    return oss.str();
}

std::string MarketTick::get_formatted_timestamp() const {
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

} // namespace trading