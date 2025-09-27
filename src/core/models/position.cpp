#include "position.hpp"
#include <stdexcept>
#include <cmath>

namespace trading {

Position::Position(const std::string& instrument_symbol)
    : instrument_symbol_(instrument_symbol), quantity_(0.0), average_price_(0.0),
      realized_pnl_(0.0), unrealized_pnl_(0.0),
      last_updated_(std::chrono::system_clock::now()) {

    if (instrument_symbol.empty()) {
        throw std::invalid_argument("Instrument symbol cannot be empty");
    }
}

double Position::get_quantity() const {
    std::lock_guard<std::mutex> lock(position_mutex_);
    return quantity_;
}

double Position::get_average_price() const {
    std::lock_guard<std::mutex> lock(position_mutex_);
    return average_price_;
}

double Position::get_realized_pnl() const {
    std::lock_guard<std::mutex> lock(position_mutex_);
    return realized_pnl_;
}

double Position::get_unrealized_pnl() const {
    std::lock_guard<std::mutex> lock(position_mutex_);
    return unrealized_pnl_;
}

std::chrono::system_clock::time_point Position::get_last_updated() const {
    std::lock_guard<std::mutex> lock(position_mutex_);
    return last_updated_;
}

double Position::get_market_value(double current_price) const {
    std::lock_guard<std::mutex> lock(position_mutex_);
    return quantity_ * current_price;
}

double Position::get_total_pnl(double current_price) const {
    std::lock_guard<std::mutex> lock(position_mutex_);
    double unrealized = (current_price - average_price_) * quantity_;
    return realized_pnl_ + unrealized;
}

bool Position::is_flat() const {
    std::lock_guard<std::mutex> lock(position_mutex_);
    return std::abs(quantity_) < 1e-8;
}

bool Position::is_long() const {
    std::lock_guard<std::mutex> lock(position_mutex_);
    return quantity_ > 1e-8;
}

bool Position::is_short() const {
    std::lock_guard<std::mutex> lock(position_mutex_);
    return quantity_ < -1e-8;
}

void Position::add_trade(double quantity, double price) {
    std::lock_guard<std::mutex> lock(position_mutex_);

    if (price <= 0) {
        throw std::invalid_argument("Price must be positive");
    }

    double current_quantity = quantity_;
    double new_total_quantity = current_quantity + quantity;

    // Check if this trade is closing or reducing the position
    if ((current_quantity > 0 && quantity < 0) ||
        (current_quantity < 0 && quantity > 0)) {

        // Position is being reduced or closed
        double closing_quantity = std::min(std::abs(quantity), std::abs(current_quantity));

        // Calculate realized P&L for the closing portion
        if (current_quantity > 0) {
            // Closing long position
            realized_pnl_ += closing_quantity * (price - average_price_);
        } else {
            // Closing short position
            realized_pnl_ += closing_quantity * (average_price_ - price);
        }

        // Update position quantity
        quantity_ = new_total_quantity;

        // If position still exists, keep the same average price
        // If position is flat or reversed, calculate new average price
        if (std::abs(new_total_quantity) < 1e-8) {
            // Position is now flat
            quantity_ = 0.0;
            average_price_ = 0.0;
        } else if ((current_quantity > 0 && new_total_quantity < 0) ||
                   (current_quantity < 0 && new_total_quantity > 0)) {
            // Position has reversed
            average_price_ = price;
        }
        // else: position reduced but same direction, keep average_price_

    } else {
        // Position is being increased in the same direction
        if (std::abs(current_quantity) < 1e-8) {
            // Opening new position
            quantity_ = quantity;
            average_price_ = price;
        } else {
            // Adding to existing position - calculate weighted average price
            recalculate_average_price(quantity, price);
            quantity_ = new_total_quantity;
        }
    }

    update_last_modified();
}

void Position::update_unrealized_pnl(double current_price) {
    std::lock_guard<std::mutex> lock(position_mutex_);

    if (current_price <= 0) {
        throw std::invalid_argument("Current price must be positive");
    }

    if (std::abs(quantity_) > 1e-8 && average_price_ > 0) {
        unrealized_pnl_ = (current_price - average_price_) * quantity_;
    } else {
        unrealized_pnl_ = 0.0;
    }

    update_last_modified();
}

void Position::close_position() {
    std::lock_guard<std::mutex> lock(position_mutex_);

    quantity_ = 0.0;
    average_price_ = 0.0;
    unrealized_pnl_ = 0.0;
    // Note: realized_pnl_ is preserved as it represents historical performance

    update_last_modified();
}

bool Position::is_valid() const {
    std::lock_guard<std::mutex> lock(position_mutex_);

    // If position exists, average price must be positive
    if (std::abs(quantity_) > 1e-8 && average_price_ <= 0) {
        return false;
    }

    // If position is flat, average price should be zero
    if (std::abs(quantity_) < 1e-8 && average_price_ != 0) {
        return false;
    }

    return !instrument_symbol_.empty();
}

void Position::update_last_modified() {
    last_updated_ = std::chrono::system_clock::now();
}

void Position::recalculate_average_price(double new_quantity, double new_price) {
    // Calculate volume-weighted average price
    double current_value = quantity_ * average_price_;
    double new_value = new_quantity * new_price;
    double total_quantity = quantity_ + new_quantity;

    if (std::abs(total_quantity) > 1e-8) {
        average_price_ = (current_value + new_value) / total_quantity;
    }
}

double Position::calculate_realized_pnl(double closing_quantity, double closing_price) {
    if (quantity_ > 0) {
        // Closing long position
        return closing_quantity * (closing_price - average_price_);
    } else {
        // Closing short position
        return closing_quantity * (average_price_ - closing_price);
    }
}

} // namespace trading