#include "order.hpp"
#include <stdexcept>

namespace trading {

Order::Order(const std::string& order_id, const std::string& instrument_symbol,
             OrderSide side, OrderType type, double quantity, double price)
    : order_id_(order_id), instrument_symbol_(instrument_symbol),
      side_(side), type_(type), quantity_(quantity), price_(price),
      created_time_(std::chrono::system_clock::now()),
      status_(OrderStatus::NEW), filled_quantity_(0.0), total_fill_value_(0.0),
      last_modified_(created_time_) {

    if (order_id.empty()) {
        throw std::invalid_argument("Order ID cannot be empty");
    }
    if (instrument_symbol.empty()) {
        throw std::invalid_argument("Instrument symbol cannot be empty");
    }
    if (quantity <= 0) {
        throw std::invalid_argument("Quantity must be positive");
    }
    if (type == OrderType::LIMIT && price <= 0) {
        throw std::invalid_argument("Limit orders must have positive price");
    }
    if (type == OrderType::MARKET && price != 0) {
        throw std::invalid_argument("Market orders should have zero price");
    }
}

OrderStatus Order::get_status() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return status_;
}

double Order::get_filled_quantity() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return filled_quantity_;
}

double Order::get_remaining_quantity() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return quantity_ - filled_quantity_;
}

std::chrono::system_clock::time_point Order::get_last_modified() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return last_modified_;
}

const std::string& Order::get_rejection_reason() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return rejection_reason_;
}

double Order::get_average_fill_price() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (filled_quantity_ > 0) {
        return total_fill_value_ / filled_quantity_;
    }
    return 0.0;
}

bool Order::is_fully_filled() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return filled_quantity_ >= quantity_;
}

bool Order::is_working() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return status_ == OrderStatus::ACCEPTED || status_ == OrderStatus::PARTIALLY_FILLED;
}

bool Order::is_cancelable() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return is_working();
}

bool Order::accept() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (status_ == OrderStatus::NEW) {
        status_ = OrderStatus::ACCEPTED;
        update_last_modified();
        return true;
    }
    return false;
}

bool Order::reject(const std::string& reason) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (status_ == OrderStatus::NEW || status_ == OrderStatus::ACCEPTED) {
        status_ = OrderStatus::REJECTED;
        rejection_reason_ = reason;
        update_last_modified();
        return true;
    }
    return false;
}

bool Order::cancel() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (is_working()) {
        status_ = OrderStatus::CANCELED;
        update_last_modified();
        return true;
    }
    return false;
}

bool Order::fill(double quantity, double price) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (!is_working()) {
        return false;
    }

    if (quantity <= 0 || price <= 0) {
        return false;
    }

    double remaining = quantity_ - filled_quantity_;
    if (quantity > remaining) {
        return false; // Cannot fill more than remaining
    }

    filled_quantity_ += quantity;
    total_fill_value_ += quantity * price;

    if (filled_quantity_ >= quantity_) {
        status_ = OrderStatus::FILLED;
    } else {
        status_ = OrderStatus::PARTIALLY_FILLED;
    }

    update_last_modified();
    return true;
}

bool Order::partial_fill(double quantity, double price) {
    return fill(quantity, price);
}

bool Order::is_valid() const {
    return !order_id_.empty() &&
           !instrument_symbol_.empty() &&
           quantity_ > 0 &&
           (type_ == OrderType::MARKET || price_ > 0);
}

bool Order::is_status_transition_valid(OrderStatus new_status) const {
    std::lock_guard<std::mutex> lock(state_mutex_);

    switch (status_) {
        case OrderStatus::NEW:
            return new_status == OrderStatus::ACCEPTED ||
                   new_status == OrderStatus::REJECTED;

        case OrderStatus::ACCEPTED:
            return new_status == OrderStatus::PARTIALLY_FILLED ||
                   new_status == OrderStatus::FILLED ||
                   new_status == OrderStatus::CANCELED ||
                   new_status == OrderStatus::REJECTED;

        case OrderStatus::PARTIALLY_FILLED:
            return new_status == OrderStatus::FILLED ||
                   new_status == OrderStatus::CANCELED;

        case OrderStatus::FILLED:
        case OrderStatus::CANCELED:
        case OrderStatus::REJECTED:
            return false; // Terminal states

        default:
            return false;
    }
}

void Order::update_last_modified() {
    last_modified_ = std::chrono::system_clock::now();
}

bool Order::is_terminal_status(OrderStatus status) const {
    return status == OrderStatus::FILLED ||
           status == OrderStatus::CANCELED ||
           status == OrderStatus::REJECTED;
}

// Helper functions
std::string order_side_to_string(OrderSide side) {
    switch (side) {
        case OrderSide::BUY: return "BUY";
        case OrderSide::SELL: return "SELL";
        default: return "UNKNOWN";
    }
}

std::string order_type_to_string(OrderType type) {
    switch (type) {
        case OrderType::MARKET: return "MARKET";
        case OrderType::LIMIT: return "LIMIT";
        default: return "UNKNOWN";
    }
}

std::string order_status_to_string(OrderStatus status) {
    switch (status) {
        case OrderStatus::NEW: return "NEW";
        case OrderStatus::ACCEPTED: return "ACCEPTED";
        case OrderStatus::PARTIALLY_FILLED: return "PARTIALLY_FILLED";
        case OrderStatus::FILLED: return "FILLED";
        case OrderStatus::CANCELED: return "CANCELED";
        case OrderStatus::REJECTED: return "REJECTED";
        default: return "UNKNOWN";
    }
}

OrderSide string_to_order_side(const std::string& side_str) {
    if (side_str == "BUY") return OrderSide::BUY;
    if (side_str == "SELL") return OrderSide::SELL;
    throw std::invalid_argument("Unknown order side: " + side_str);
}

OrderType string_to_order_type(const std::string& type_str) {
    if (type_str == "MARKET") return OrderType::MARKET;
    if (type_str == "LIMIT") return OrderType::LIMIT;
    throw std::invalid_argument("Unknown order type: " + type_str);
}

OrderStatus string_to_order_status(const std::string& status_str) {
    if (status_str == "NEW") return OrderStatus::NEW;
    if (status_str == "ACCEPTED") return OrderStatus::ACCEPTED;
    if (status_str == "PARTIALLY_FILLED") return OrderStatus::PARTIALLY_FILLED;
    if (status_str == "FILLED") return OrderStatus::FILLED;
    if (status_str == "CANCELED") return OrderStatus::CANCELED;
    if (status_str == "REJECTED") return OrderStatus::REJECTED;
    throw std::invalid_argument("Unknown order status: " + status_str);
}

} // namespace trading