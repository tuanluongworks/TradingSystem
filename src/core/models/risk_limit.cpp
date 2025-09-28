#include "risk_limit.hpp"
#include "order.hpp"
#include "position.hpp"
#include <stdexcept>
#include <sstream>
#include <cmath>

namespace trading {

RiskLimit::RiskLimit(const std::string& instrument_symbol, LimitType type, double max_value)
    : instrument_symbol_(instrument_symbol), type_(type), max_value_(max_value), is_active_(true) {

    if (max_value <= 0) {
        throw std::invalid_argument("Max value must be positive");
    }
}

RiskLimit::RiskLimit(LimitType type, double max_value)
    : instrument_symbol_(""), type_(type), max_value_(max_value), is_active_(true) {

    if (max_value <= 0) {
        throw std::invalid_argument("Max value must be positive");
    }
}

void RiskLimit::set_max_value(double max_value) {
    if (max_value <= 0) {
        throw std::invalid_argument("Max value must be positive");
    }
    max_value_ = max_value;
}

bool RiskLimit::check_order(const OrderRequest& request) const {
    if (!is_active_) return true;

    // Check if this limit applies to the instrument
    if (!is_global() && instrument_symbol_ != request.instrument_symbol) {
        return true;
    }

    switch (type_) {
        case LimitType::MAX_ORDER_SIZE:
            return check_order_size_limit(request.quantity);

        case LimitType::MAX_POSITION_SIZE:
        case LimitType::MAX_DAILY_VOLUME:
        case LimitType::MAX_LOSS_LIMIT:
            // These require additional context (position, daily volume, daily P&L)
            return true; // Cannot validate without additional data

        default:
            return false;
    }
}

bool RiskLimit::check_order_with_position(const OrderRequest& request, const Position* current_position) const {
    if (!is_active_) return true;

    // Check if this limit applies to the instrument
    if (!is_global() && instrument_symbol_ != request.instrument_symbol) {
        return true;
    }

    switch (type_) {
        case LimitType::MAX_ORDER_SIZE:
            return check_order_size_limit(request.quantity);

        case LimitType::MAX_POSITION_SIZE: {
            double current_quantity = current_position ? current_position->get_quantity() : 0.0;
            double order_quantity = (request.side == OrderSide::BUY) ? request.quantity : -request.quantity;
            return check_position_limit(current_quantity, order_quantity);
        }

        case LimitType::MAX_DAILY_VOLUME:
        case LimitType::MAX_LOSS_LIMIT:
            // These require additional context (daily volume, daily P&L)
            return true; // Cannot validate without additional data

        default:
            return false;
    }
}

bool RiskLimit::check_position_limit(double current_quantity, double order_quantity) const {
    double resulting_position = std::abs(current_quantity + order_quantity);
    return resulting_position <= max_value_;
}

bool RiskLimit::check_order_size_limit(double order_quantity) const {
    return std::abs(order_quantity) <= max_value_;
}

bool RiskLimit::check_daily_volume_limit(double current_daily_volume, double order_quantity) const {
    return (current_daily_volume + std::abs(order_quantity)) <= max_value_;
}

bool RiskLimit::check_daily_loss_limit(double current_daily_pnl) const {
    // Negative P&L means loss
    return current_daily_pnl >= -max_value_;
}

std::string RiskLimit::get_violation_reason(const OrderRequest& request) const {
    if (!is_active_) return "";

    // Check if this limit applies to the instrument
    if (!is_global() && instrument_symbol_ != request.instrument_symbol) {
        return "";
    }

    std::ostringstream oss;

    switch (type_) {
        case LimitType::MAX_ORDER_SIZE:
            if (!check_order_size_limit(request.quantity)) {
                oss << "Order size " << request.quantity
                    << " exceeds maximum order size limit of " << max_value_;
                if (!is_global()) {
                    oss << " for " << instrument_symbol_;
                }
            }
            break;

        case LimitType::MAX_POSITION_SIZE:
        case LimitType::MAX_DAILY_VOLUME:
        case LimitType::MAX_LOSS_LIMIT:
            oss << "Cannot validate " << limit_type_to_string(type_) << " without additional context";
            break;

        default:
            oss << "Unknown limit type";
            break;
    }

    return oss.str();
}

std::string RiskLimit::get_violation_reason_with_position(const OrderRequest& request, const Position* current_position) const {
    if (!is_active_) return "";

    // Check if this limit applies to the instrument
    if (!is_global() && instrument_symbol_ != request.instrument_symbol) {
        return "";
    }

    std::ostringstream oss;

    switch (type_) {
        case LimitType::MAX_ORDER_SIZE:
            if (!check_order_size_limit(request.quantity)) {
                oss << "Order size " << request.quantity
                    << " exceeds maximum order size limit of " << max_value_;
                if (!is_global()) {
                    oss << " for " << instrument_symbol_;
                }
            }
            break;

        case LimitType::MAX_POSITION_SIZE: {
            double current_quantity = current_position ? current_position->get_quantity() : 0.0;
            double order_quantity = (request.side == OrderSide::BUY) ? request.quantity : -request.quantity;
            if (!check_position_limit(current_quantity, order_quantity)) {
                double resulting_position = std::abs(current_quantity + order_quantity);
                oss << "Resulting position " << resulting_position
                    << " would exceed maximum position limit of " << max_value_;
                if (!is_global()) {
                    oss << " for " << instrument_symbol_;
                }
            }
            break;
        }

        case LimitType::MAX_DAILY_VOLUME:
        case LimitType::MAX_LOSS_LIMIT:
            oss << "Cannot validate " << limit_type_to_string(type_) << " without additional context";
            break;

        default:
            oss << "Unknown limit type";
            break;
    }

    return oss.str();
}

bool RiskLimit::is_valid() const {
    return max_value_ > 0;
}

bool RiskLimit::operator==(const RiskLimit& other) const {
    return instrument_symbol_ == other.instrument_symbol_ &&
           type_ == other.type_ &&
           std::abs(max_value_ - other.max_value_) < 1e-8 &&
           is_active_ == other.is_active_;
}

bool RiskLimit::operator<(const RiskLimit& other) const {
    // Sort by instrument symbol first, then by limit type
    if (instrument_symbol_ != other.instrument_symbol_) {
        return instrument_symbol_ < other.instrument_symbol_;
    }
    return static_cast<int>(type_) < static_cast<int>(other.type_);
}

std::string RiskLimit::get_limit_description() const {
    std::ostringstream oss;
    oss << limit_type_to_string(type_) << " = " << max_value_;
    if (!is_global()) {
        oss << " for " << instrument_symbol_;
    } else {
        oss << " (global)";
    }
    if (!is_active_) {
        oss << " (inactive)";
    }
    return oss.str();
}

// Helper functions
std::string limit_type_to_string(LimitType type) {
    switch (type) {
        case LimitType::MAX_POSITION_SIZE: return "MAX_POSITION_SIZE";
        case LimitType::MAX_ORDER_SIZE: return "MAX_ORDER_SIZE";
        case LimitType::MAX_DAILY_VOLUME: return "MAX_DAILY_VOLUME";
        case LimitType::MAX_LOSS_LIMIT: return "MAX_LOSS_LIMIT";
        default: return "UNKNOWN";
    }
}

LimitType string_to_limit_type(const std::string& type_str) {
    if (type_str == "MAX_POSITION_SIZE") return LimitType::MAX_POSITION_SIZE;
    if (type_str == "MAX_ORDER_SIZE") return LimitType::MAX_ORDER_SIZE;
    if (type_str == "MAX_DAILY_VOLUME") return LimitType::MAX_DAILY_VOLUME;
    if (type_str == "MAX_LOSS_LIMIT") return LimitType::MAX_LOSS_LIMIT;
    throw std::invalid_argument("Unknown limit type: " + type_str);
}

} // namespace trading