#include "instrument.hpp"
#include <mutex>
#include <cmath>
#include <stdexcept>

namespace trading {

Instrument::Instrument(const std::string& symbol, const std::string& name,
                       InstrumentType type, double tick_size, int lot_size)
    : symbol_(symbol), name_(name), type_(type), tick_size_(tick_size),
      lot_size_(lot_size), is_active_(true), bid_price_(0.0), ask_price_(0.0),
      last_price_(0.0), last_update_(std::chrono::system_clock::now()) {

    if (symbol.empty()) {
        throw std::invalid_argument("Symbol cannot be empty");
    }
    if (tick_size <= 0.0) {
        throw std::invalid_argument("Tick size must be positive");
    }
    if (lot_size <= 0) {
        throw std::invalid_argument("Lot size must be positive");
    }
}

void Instrument::update_market_data(double bid, double ask, double last) {
    std::lock_guard<std::mutex> lock(market_data_mutex_);

    if (bid < 0 || ask < 0 || last < 0) {
        throw std::invalid_argument("Prices must be non-negative");
    }
    if (ask > 0 && bid > 0 && ask < bid) {
        throw std::invalid_argument("Ask price must be >= bid price");
    }

    bid_price_ = bid;
    ask_price_ = ask;
    last_price_ = last;
    last_update_ = std::chrono::system_clock::now();
}

bool Instrument::is_valid() const {
    return !symbol_.empty() &&
           tick_size_ > 0.0 &&
           lot_size_ > 0;
}

bool Instrument::is_price_valid(double price) const {
    if (price < 0) return false;

    // Check if price aligns with tick size
    double remainder = std::fmod(price, tick_size_);
    return std::abs(remainder) < 1e-8 || std::abs(remainder - tick_size_) < 1e-8;
}

bool Instrument::is_quantity_valid(int quantity) const {
    return quantity > 0 && (quantity % lot_size_) == 0;
}

double Instrument::round_to_tick_size(double price) const {
    if (tick_size_ <= 0) return price;
    return std::round(price / tick_size_) * tick_size_;
}

int Instrument::round_to_lot_size(int quantity) const {
    if (lot_size_ <= 0) return quantity;
    return ((quantity + lot_size_ / 2) / lot_size_) * lot_size_;
}

double Instrument::get_spread() const {
    std::lock_guard<std::mutex> lock(market_data_mutex_);
    if (ask_price_ > 0 && bid_price_ > 0) {
        return ask_price_ - bid_price_;
    }
    return 0.0;
}

double Instrument::get_mid_price() const {
    std::lock_guard<std::mutex> lock(market_data_mutex_);
    if (ask_price_ > 0 && bid_price_ > 0) {
        return (ask_price_ + bid_price_) / 2.0;
    }
    return last_price_;
}

std::string instrument_type_to_string(InstrumentType type) {
    switch (type) {
        case InstrumentType::STOCK: return "STOCK";
        case InstrumentType::FOREX: return "FOREX";
        case InstrumentType::CRYPTO: return "CRYPTO";
        case InstrumentType::COMMODITY: return "COMMODITY";
        case InstrumentType::INDEX: return "INDEX";
        default: return "UNKNOWN";
    }
}

InstrumentType string_to_instrument_type(const std::string& type_str) {
    if (type_str == "STOCK") return InstrumentType::STOCK;
    if (type_str == "FOREX") return InstrumentType::FOREX;
    if (type_str == "CRYPTO") return InstrumentType::CRYPTO;
    if (type_str == "COMMODITY") return InstrumentType::COMMODITY;
    if (type_str == "INDEX") return InstrumentType::INDEX;
    throw std::invalid_argument("Unknown instrument type: " + type_str);
}

} // namespace trading