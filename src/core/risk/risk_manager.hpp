#pragma once

#include "contracts/trading_engine_api.hpp"

namespace trading {

// Placeholder - will be implemented in Phase 3.5
class RiskManager : public IRiskManager {
public:
    bool validate_order(const OrderRequest&) const override { return false; }
    std::string get_rejection_reason(const OrderRequest&) const override { return ""; }
    bool set_position_limit(const std::string&, double) override { return false; }
    bool set_order_size_limit(const std::string&, double) override { return false; }
    bool set_daily_loss_limit(double) override { return false; }
    double get_position_limit(const std::string&) const override { return 0; }
    double get_order_size_limit(const std::string&) const override { return 0; }
    double get_daily_loss_limit() const override { return 0; }
    double get_current_exposure(const std::string&) const override { return 0; }
    double get_daily_pnl() const override { return 0; }
    double get_total_position_value() const override { return 0; }
};

} // namespace trading