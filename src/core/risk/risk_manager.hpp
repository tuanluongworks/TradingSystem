#pragma once

#include "../../contracts/trading_engine_api.hpp"
#include "../models/risk_limit.hpp"
#include "../models/position.hpp"
#include "../models/order.hpp"
#include "../../utils/config.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <functional>
#include <chrono>

namespace trading {

// Forward declarations
class Position;
class Order;

/**
 * Risk Manager Implementation
 * Provides pre-trade risk validation and limit enforcement
 */
class RiskManager : public IRiskManager {
public:
    explicit RiskManager(const RiskManagementConfig& config = RiskManagementConfig{});
    virtual ~RiskManager() = default;

    // Configuration
    void update_config(const RiskManagementConfig& config);
    RiskManagementConfig get_config() const;

    // IRiskManager implementation
    bool validate_order(const OrderRequest& request) const override;
    std::string get_rejection_reason(const OrderRequest& request) const override;

    bool set_position_limit(const std::string& symbol, double max_quantity) override;
    bool set_order_size_limit(const std::string& symbol, double max_quantity) override;
    bool set_daily_loss_limit(double max_loss) override;

    double get_position_limit(const std::string& symbol) const override;
    double get_order_size_limit(const std::string& symbol) const override;
    double get_daily_loss_limit() const override;

    double get_current_exposure(const std::string& symbol) const override;
    double get_daily_pnl() const override;
    double get_total_position_value() const override;

    // Position tracking
    void update_position(std::shared_ptr<Position> position);
    void remove_position(const std::string& symbol);
    std::shared_ptr<Position> get_position(const std::string& symbol) const;

    // Order tracking
    void add_working_order(std::shared_ptr<Order> order);
    void remove_working_order(const std::string& order_id);
    std::vector<std::shared_ptr<Order>> get_working_orders_for_symbol(const std::string& symbol) const;

    // Risk metrics
    double calculate_position_exposure(const std::string& symbol) const;
    double calculate_order_exposure(const OrderRequest& request) const;
    double calculate_potential_position(const std::string& symbol, const OrderRequest& request) const;

    // Daily P&L tracking
    void update_daily_pnl(double realized_pnl, double unrealized_pnl);
    void reset_daily_pnl(); // Called at start of new trading day

    // Risk limit management
    void add_risk_limit(const RiskLimit& limit);
    void remove_risk_limit(const std::string& symbol, LimitType type);
    std::vector<RiskLimit> get_risk_limits(const std::string& symbol = "") const;

    // Validation methods
    bool is_position_within_limits(const std::string& symbol, double new_quantity) const;
    bool is_order_size_valid(const std::string& symbol, double quantity) const;
    bool is_daily_loss_within_limit(double additional_loss = 0.0) const;
    bool has_sufficient_buying_power(const OrderRequest& request) const;

    // Risk status
    bool is_trading_enabled() const;
    void set_trading_enabled(bool enabled);
    std::string get_risk_status() const;

private:
    mutable std::mutex risk_mutex_;
    RiskManagementConfig config_;

    // Position tracking
    std::unordered_map<std::string, std::shared_ptr<Position>> positions_;

    // Working orders tracking
    std::unordered_map<std::string, std::shared_ptr<Order>> working_orders_;
    std::unordered_map<std::string, std::vector<std::string>> orders_by_symbol_;

    // Risk limits
    std::vector<RiskLimit> risk_limits_;

    // Daily tracking
    double daily_realized_pnl_;
    double daily_unrealized_pnl_;
    std::chrono::system_clock::time_point last_pnl_update_;

    // Risk state
    bool trading_enabled_;
    mutable std::string last_rejection_reason_;

    // Helper methods
    double get_effective_position_limit(const std::string& symbol) const;
    double get_effective_order_size_limit(const std::string& symbol) const;

    // Validation helpers
    std::string validate_order_basic(const OrderRequest& request) const;
    std::string validate_order_size(const OrderRequest& request) const;
    std::string validate_position_limits(const OrderRequest& request) const;
    std::string validate_daily_loss_limit(const OrderRequest& request) const;
    std::string validate_instrument(const OrderRequest& request) const;

    // Position calculation helpers
    double calculate_current_position_quantity(const std::string& symbol) const;
    double calculate_working_order_quantity(const std::string& symbol, OrderSide side) const;

    // Logging helpers
    void log_risk_violation(const std::string& reason, const OrderRequest& request) const;
    void log_risk_info(const std::string& message) const;
};

/**
 * Risk Validator Interface
 * Allows for pluggable risk validation strategies
 */
class IRiskValidator {
public:
    virtual ~IRiskValidator() = default;
    virtual bool validate(const OrderRequest& request, const RiskManager& manager) const = 0;
    virtual std::string get_rejection_reason() const = 0;
};

/**
 * Composite Risk Validator
 * Chains multiple validators together
 */
class CompositeRiskValidator : public IRiskValidator {
public:
    void add_validator(std::unique_ptr<IRiskValidator> validator);
    bool validate(const OrderRequest& request, const RiskManager& manager) const override;
    std::string get_rejection_reason() const override;

private:
    std::vector<std::unique_ptr<IRiskValidator>> validators_;
    mutable std::string rejection_reason_;
};

/**
 * Built-in Risk Validators
 */

class PositionLimitValidator : public IRiskValidator {
public:
    bool validate(const OrderRequest& request, const RiskManager& manager) const override;
    std::string get_rejection_reason() const override;
private:
    mutable std::string rejection_reason_;
};

class OrderSizeValidator : public IRiskValidator {
public:
    bool validate(const OrderRequest& request, const RiskManager& manager) const override;
    std::string get_rejection_reason() const override;
private:
    mutable std::string rejection_reason_;
};

class DailyLossValidator : public IRiskValidator {
public:
    bool validate(const OrderRequest& request, const RiskManager& manager) const override;
    std::string get_rejection_reason() const override;
private:
    mutable std::string rejection_reason_;
};

} // namespace trading