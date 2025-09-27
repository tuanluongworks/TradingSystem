#include "risk_manager.hpp"
#include "../../utils/logging.hpp"
#include "../../utils/exceptions.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace trading {

// RiskManager implementation

RiskManager::RiskManager(const RiskManagementConfig& config)
    : config_(config)
    , daily_realized_pnl_(0.0)
    , daily_unrealized_pnl_(0.0)
    , last_pnl_update_(std::chrono::system_clock::now())
    , trading_enabled_(true) {

    // Initialize default risk limits from config
    if (config_.enable_risk_checks) {
        // Add global limits
        add_risk_limit(RiskLimit("", LimitType::MAX_POSITION_SIZE, config_.max_position_size, true));
        add_risk_limit(RiskLimit("", LimitType::MAX_ORDER_SIZE, config_.max_order_size, true));
        add_risk_limit(RiskLimit("", LimitType::MAX_LOSS_LIMIT, config_.max_daily_loss, true));

        // Add symbol-specific limits
        for (const auto& [symbol, limit] : config_.symbol_position_limits) {
            add_risk_limit(RiskLimit(symbol, LimitType::MAX_POSITION_SIZE, limit, true));
        }
        for (const auto& [symbol, limit] : config_.symbol_order_limits) {
            add_risk_limit(RiskLimit(symbol, LimitType::MAX_ORDER_SIZE, limit, true));
        }
    }

    log_risk_info("Risk Manager initialized with " + std::to_string(risk_limits_.size()) + " risk limits");
}

void RiskManager::update_config(const RiskManagementConfig& config) {
    std::lock_guard<std::mutex> lock(risk_mutex_);
    config_ = config;

    // Rebuild risk limits
    risk_limits_.clear();
    if (config_.enable_risk_checks) {
        add_risk_limit(RiskLimit("", LimitType::MAX_POSITION_SIZE, config_.max_position_size, true));
        add_risk_limit(RiskLimit("", LimitType::MAX_ORDER_SIZE, config_.max_order_size, true));
        add_risk_limit(RiskLimit("", LimitType::MAX_LOSS_LIMIT, config_.max_daily_loss, true));

        for (const auto& [symbol, limit] : config_.symbol_position_limits) {
            add_risk_limit(RiskLimit(symbol, LimitType::MAX_POSITION_SIZE, limit, true));
        }
        for (const auto& [symbol, limit] : config_.symbol_order_limits) {
            add_risk_limit(RiskLimit(symbol, LimitType::MAX_ORDER_SIZE, limit, true));
        }
    }

    log_risk_info("Risk configuration updated");
}

RiskManagementConfig RiskManager::get_config() const {
    std::lock_guard<std::mutex> lock(risk_mutex_);
    return config_;
}

bool RiskManager::validate_order(const OrderRequest& request) const {
    if (!config_.enable_risk_checks) {
        return true;
    }

    if (!trading_enabled_) {
        last_rejection_reason_ = "Trading is disabled";
        return false;
    }

    std::lock_guard<std::mutex> lock(risk_mutex_);

    // Run all validation checks
    std::string error = validate_order_basic(request);
    if (!error.empty()) {
        last_rejection_reason_ = error;
        log_risk_violation(error, request);
        return false;
    }

    error = validate_order_size(request);
    if (!error.empty()) {
        last_rejection_reason_ = error;
        log_risk_violation(error, request);
        return false;
    }

    error = validate_position_limits(request);
    if (!error.empty()) {
        last_rejection_reason_ = error;
        log_risk_violation(error, request);
        return false;
    }

    error = validate_daily_loss_limit(request);
    if (!error.empty()) {
        last_rejection_reason_ = error;
        log_risk_violation(error, request);
        return false;
    }

    error = validate_instrument(request);
    if (!error.empty()) {
        last_rejection_reason_ = error;
        log_risk_violation(error, request);
        return false;
    }

    last_rejection_reason_.clear();
    return true;
}

std::string RiskManager::get_rejection_reason(const OrderRequest& request) const {
    // Call validate_order to populate the rejection reason
    validate_order(request);
    return last_rejection_reason_;
}

bool RiskManager::set_position_limit(const std::string& symbol, double max_quantity) {
    std::lock_guard<std::mutex> lock(risk_mutex_);

    if (max_quantity <= 0) {
        return false;
    }

    // Remove existing limit for this symbol
    remove_risk_limit(symbol, LimitType::MAX_POSITION_SIZE);

    // Add new limit
    add_risk_limit(RiskLimit(symbol, LimitType::MAX_POSITION_SIZE, max_quantity, true));

    // Update config
    if (symbol.empty()) {
        config_.max_position_size = max_quantity;
    } else {
        config_.symbol_position_limits[symbol] = max_quantity;
    }

    log_risk_info("Position limit updated for " + (symbol.empty() ? "global" : symbol) +
                  ": " + std::to_string(max_quantity));
    return true;
}

bool RiskManager::set_order_size_limit(const std::string& symbol, double max_quantity) {
    std::lock_guard<std::mutex> lock(risk_mutex_);

    if (max_quantity <= 0) {
        return false;
    }

    // Remove existing limit for this symbol
    remove_risk_limit(symbol, LimitType::MAX_ORDER_SIZE);

    // Add new limit
    add_risk_limit(RiskLimit(symbol, LimitType::MAX_ORDER_SIZE, max_quantity, true));

    // Update config
    if (symbol.empty()) {
        config_.max_order_size = max_quantity;
    } else {
        config_.symbol_order_limits[symbol] = max_quantity;
    }

    log_risk_info("Order size limit updated for " + (symbol.empty() ? "global" : symbol) +
                  ": " + std::to_string(max_quantity));
    return true;
}

bool RiskManager::set_daily_loss_limit(double max_loss) {
    std::lock_guard<std::mutex> lock(risk_mutex_);

    if (max_loss <= 0) {
        return false;
    }

    // Remove existing daily loss limit
    remove_risk_limit("", LimitType::MAX_LOSS_LIMIT);

    // Add new limit
    add_risk_limit(RiskLimit("", LimitType::MAX_LOSS_LIMIT, max_loss, true));

    // Update config
    config_.max_daily_loss = max_loss;

    log_risk_info("Daily loss limit updated: " + std::to_string(max_loss));
    return true;
}

double RiskManager::get_position_limit(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(risk_mutex_);
    return get_effective_position_limit(symbol);
}

double RiskManager::get_order_size_limit(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(risk_mutex_);
    return get_effective_order_size_limit(symbol);
}

double RiskManager::get_daily_loss_limit() const {
    std::lock_guard<std::mutex> lock(risk_mutex_);
    return config_.max_daily_loss;
}

double RiskManager::get_current_exposure(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(risk_mutex_);
    return calculate_position_exposure(symbol);
}

double RiskManager::get_daily_pnl() const {
    std::lock_guard<std::mutex> lock(risk_mutex_);
    return daily_realized_pnl_ + daily_unrealized_pnl_;
}

double RiskManager::get_total_position_value() const {
    std::lock_guard<std::mutex> lock(risk_mutex_);

    double total_value = 0.0;
    for (const auto& [symbol, position] : positions_) {
        if (position && !position->is_flat()) {
            // Use last known market price for approximation
            // In a real system, this would get current market price
            total_value += std::abs(position->get_quantity() * position->get_average_price());
        }
    }
    return total_value;
}

// Position tracking

void RiskManager::update_position(std::shared_ptr<Position> position) {
    if (!position) return;

    std::lock_guard<std::mutex> lock(risk_mutex_);
    const std::string& symbol = position->get_instrument_symbol();

    if (position->is_flat()) {
        positions_.erase(symbol);
    } else {
        positions_[symbol] = position;
    }
}

void RiskManager::remove_position(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(risk_mutex_);
    positions_.erase(symbol);
}

std::shared_ptr<Position> RiskManager::get_position(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(risk_mutex_);
    auto it = positions_.find(symbol);
    return (it != positions_.end()) ? it->second : nullptr;
}

// Order tracking

void RiskManager::add_working_order(std::shared_ptr<Order> order) {
    if (!order) return;

    std::lock_guard<std::mutex> lock(risk_mutex_);
    const std::string& order_id = order->get_order_id();
    const std::string& symbol = order->get_instrument_symbol();

    working_orders_[order_id] = order;
    orders_by_symbol_[symbol].push_back(order_id);
}

void RiskManager::remove_working_order(const std::string& order_id) {
    std::lock_guard<std::mutex> lock(risk_mutex_);

    auto it = working_orders_.find(order_id);
    if (it != working_orders_.end()) {
        const std::string& symbol = it->second->get_instrument_symbol();
        working_orders_.erase(it);

        // Remove from symbol index
        auto& order_list = orders_by_symbol_[symbol];
        order_list.erase(std::remove(order_list.begin(), order_list.end(), order_id), order_list.end());

        if (order_list.empty()) {
            orders_by_symbol_.erase(symbol);
        }
    }
}

std::vector<std::shared_ptr<Order>> RiskManager::get_working_orders_for_symbol(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(risk_mutex_);
    std::vector<std::shared_ptr<Order>> orders;

    auto it = orders_by_symbol_.find(symbol);
    if (it != orders_by_symbol_.end()) {
        for (const std::string& order_id : it->second) {
            auto order_it = working_orders_.find(order_id);
            if (order_it != working_orders_.end()) {
                orders.push_back(order_it->second);
            }
        }
    }

    return orders;
}

// Risk metrics

double RiskManager::calculate_position_exposure(const std::string& symbol) const {
    auto position = get_position(symbol);
    if (!position) {
        return 0.0;
    }

    // Return absolute quantity as exposure
    return std::abs(position->get_quantity());
}

double RiskManager::calculate_order_exposure(const OrderRequest& request) const {
    return request.quantity; // Simple implementation - quantity as exposure
}

double RiskManager::calculate_potential_position(const std::string& symbol, const OrderRequest& request) const {
    double current_position = calculate_current_position_quantity(symbol);
    double order_impact = (request.side == OrderSide::BUY) ? request.quantity : -request.quantity;

    return current_position + order_impact;
}

// Daily P&L tracking

void RiskManager::update_daily_pnl(double realized_pnl, double unrealized_pnl) {
    std::lock_guard<std::mutex> lock(risk_mutex_);
    daily_realized_pnl_ = realized_pnl;
    daily_unrealized_pnl_ = unrealized_pnl;
    last_pnl_update_ = std::chrono::system_clock::now();
}

void RiskManager::reset_daily_pnl() {
    std::lock_guard<std::mutex> lock(risk_mutex_);
    daily_realized_pnl_ = 0.0;
    daily_unrealized_pnl_ = 0.0;
    last_pnl_update_ = std::chrono::system_clock::now();
    log_risk_info("Daily P&L reset");
}

// Risk limit management

void RiskManager::add_risk_limit(const RiskLimit& limit) {
    // Remove any existing limit of the same type for the same symbol
    remove_risk_limit(limit.get_instrument_symbol(), limit.get_type());

    risk_limits_.push_back(limit);
}

void RiskManager::remove_risk_limit(const std::string& symbol, LimitType type) {
    risk_limits_.erase(
        std::remove_if(risk_limits_.begin(), risk_limits_.end(),
            [&symbol, type](const RiskLimit& limit) {
                return limit.get_instrument_symbol() == symbol && limit.get_type() == type;
            }),
        risk_limits_.end());
}

std::vector<RiskLimit> RiskManager::get_risk_limits(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(risk_mutex_);
    std::vector<RiskLimit> limits;

    for (const auto& limit : risk_limits_) {
        if (symbol.empty() || limit.get_instrument_symbol() == symbol) {
            limits.push_back(limit);
        }
    }

    return limits;
}

// Validation methods

bool RiskManager::is_position_within_limits(const std::string& symbol, double new_quantity) const {
    double limit = get_effective_position_limit(symbol);
    return std::abs(new_quantity) <= limit;
}

bool RiskManager::is_order_size_valid(const std::string& symbol, double quantity) const {
    double limit = get_effective_order_size_limit(symbol);
    return quantity <= limit;
}

bool RiskManager::is_daily_loss_within_limit(double additional_loss) const {
    double total_loss = get_daily_pnl() - additional_loss; // Negative P&L is loss
    return total_loss >= -config_.max_daily_loss;
}

bool RiskManager::has_sufficient_buying_power(const OrderRequest& request) const {
    // Simplified implementation - in a real system this would check account balance
    // For now, assume sufficient buying power if within position limits
    double potential_position = calculate_potential_position(request.instrument_symbol, request);
    return is_position_within_limits(request.instrument_symbol, potential_position);
}

// Risk status

bool RiskManager::is_trading_enabled() const {
    std::lock_guard<std::mutex> lock(risk_mutex_);
    return trading_enabled_;
}

void RiskManager::set_trading_enabled(bool enabled) {
    std::lock_guard<std::mutex> lock(risk_mutex_);
    trading_enabled_ = enabled;
    log_risk_info("Trading " + std::string(enabled ? "enabled" : "disabled"));
}

std::string RiskManager::get_risk_status() const {
    std::lock_guard<std::mutex> lock(risk_mutex_);

    std::stringstream ss;
    ss << "Trading: " << (trading_enabled_ ? "Enabled" : "Disabled");
    ss << ", Daily P&L: " << get_daily_pnl();
    ss << ", Positions: " << positions_.size();
    ss << ", Working Orders: " << working_orders_.size();
    ss << ", Risk Limits: " << risk_limits_.size();

    return ss.str();
}

// Helper methods

double RiskManager::get_effective_position_limit(const std::string& symbol) const {
    // Check for symbol-specific limit first
    for (const auto& limit : risk_limits_) {
        if (limit.get_instrument_symbol() == symbol &&
            limit.get_type() == LimitType::MAX_POSITION_SIZE &&
            limit.is_active()) {
            return limit.get_max_value();
        }
    }

    // Fall back to global limit
    for (const auto& limit : risk_limits_) {
        if (limit.get_instrument_symbol().empty() &&
            limit.get_type() == LimitType::MAX_POSITION_SIZE &&
            limit.is_active()) {
            return limit.get_max_value();
        }
    }

    return config_.max_position_size;
}

double RiskManager::get_effective_order_size_limit(const std::string& symbol) const {
    // Check for symbol-specific limit first
    for (const auto& limit : risk_limits_) {
        if (limit.get_instrument_symbol() == symbol &&
            limit.get_type() == LimitType::MAX_ORDER_SIZE &&
            limit.is_active()) {
            return limit.get_max_value();
        }
    }

    // Fall back to global limit
    for (const auto& limit : risk_limits_) {
        if (limit.get_instrument_symbol().empty() &&
            limit.get_type() == LimitType::MAX_ORDER_SIZE &&
            limit.is_active()) {
            return limit.get_max_value();
        }
    }

    return config_.max_order_size;
}

// Validation helpers

std::string RiskManager::validate_order_basic(const OrderRequest& request) const {
    if (request.instrument_symbol.empty()) {
        return "Invalid instrument symbol";
    }

    if (request.quantity <= 0) {
        return "Invalid order quantity";
    }

    if (request.type == OrderType::LIMIT && request.price <= 0) {
        return "Invalid limit price";
    }

    return "";
}

std::string RiskManager::validate_order_size(const OrderRequest& request) const {
    double limit = get_effective_order_size_limit(request.instrument_symbol);
    if (request.quantity > limit) {
        return "Order size " + std::to_string(request.quantity) +
               " exceeds limit " + std::to_string(limit);
    }
    return "";
}

std::string RiskManager::validate_position_limits(const OrderRequest& request) const {
    double potential_position = calculate_potential_position(request.instrument_symbol, request);

    if (!is_position_within_limits(request.instrument_symbol, potential_position)) {
        double limit = get_effective_position_limit(request.instrument_symbol);
        return "Potential position " + std::to_string(std::abs(potential_position)) +
               " exceeds limit " + std::to_string(limit);
    }

    return "";
}

std::string RiskManager::validate_daily_loss_limit(const OrderRequest& request) const {
    // Simplified risk estimation - in reality would be more sophisticated
    double estimated_risk = request.quantity * 0.1; // Assume 10% potential loss

    if (!is_daily_loss_within_limit(estimated_risk)) {
        return "Order would exceed daily loss limit";
    }

    return "";
}

std::string RiskManager::validate_instrument(const OrderRequest& request) const {
    // Placeholder - in a real system would validate against active instruments
    if (request.instrument_symbol.length() < 2) {
        return "Invalid instrument symbol format";
    }

    return "";
}

// Position calculation helpers

double RiskManager::calculate_current_position_quantity(const std::string& symbol) const {
    auto position = get_position(symbol);
    return position ? position->get_quantity() : 0.0;
}

double RiskManager::calculate_working_order_quantity(const std::string& symbol, OrderSide side) const {
    double total = 0.0;

    auto orders = get_working_orders_for_symbol(symbol);
    for (const auto& order : orders) {
        if (order->get_side() == side && order->is_working()) {
            total += order->get_remaining_quantity();
        }
    }

    return total;
}

// Logging helpers

void RiskManager::log_risk_violation(const std::string& reason, const OrderRequest& request) const {
    std::string message = "Risk violation: " + reason +
                         " (Symbol: " + request.instrument_symbol +
                         ", Side: " + (request.side == OrderSide::BUY ? "BUY" : "SELL") +
                         ", Quantity: " + std::to_string(request.quantity) + ")";
    TRADING_LOG_WARN(message);
}

void RiskManager::log_risk_info(const std::string& message) const {
    TRADING_LOG_INFO("RiskManager: " + message);
}

// Risk Validator implementations

void CompositeRiskValidator::add_validator(std::unique_ptr<IRiskValidator> validator) {
    validators_.push_back(std::move(validator));
}

bool CompositeRiskValidator::validate(const OrderRequest& request, const RiskManager& manager) const {
    for (const auto& validator : validators_) {
        if (!validator->validate(request, manager)) {
            rejection_reason_ = validator->get_rejection_reason();
            return false;
        }
    }
    rejection_reason_.clear();
    return true;
}

std::string CompositeRiskValidator::get_rejection_reason() const {
    return rejection_reason_;
}

// Built-in validators

bool PositionLimitValidator::validate(const OrderRequest& request, const RiskManager& manager) const {
    double potential_position = manager.calculate_potential_position(request.instrument_symbol, request);

    if (!manager.is_position_within_limits(request.instrument_symbol, potential_position)) {
        rejection_reason_ = "Position limit exceeded";
        return false;
    }

    rejection_reason_.clear();
    return true;
}

std::string PositionLimitValidator::get_rejection_reason() const {
    return rejection_reason_;
}

bool OrderSizeValidator::validate(const OrderRequest& request, const RiskManager& manager) const {
    if (!manager.is_order_size_valid(request.instrument_symbol, request.quantity)) {
        rejection_reason_ = "Order size limit exceeded";
        return false;
    }

    rejection_reason_.clear();
    return true;
}

std::string OrderSizeValidator::get_rejection_reason() const {
    return rejection_reason_;
}

bool DailyLossValidator::validate(const OrderRequest& request, const RiskManager& manager) const {
    // Estimate potential loss from this order
    double estimated_risk = request.quantity * 0.05; // 5% estimated risk

    if (!manager.is_daily_loss_within_limit(estimated_risk)) {
        rejection_reason_ = "Daily loss limit would be exceeded";
        return false;
    }

    rejection_reason_.clear();
    return true;
}

std::string DailyLossValidator::get_rejection_reason() const {
    return rejection_reason_;
}

} // namespace trading