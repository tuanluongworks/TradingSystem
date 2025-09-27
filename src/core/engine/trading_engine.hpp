#pragma once

#include "contracts/trading_engine_api.hpp"

namespace trading {

// Placeholder - will be implemented in Phase 3.5
class TradingEngine : public ITradingEngine {
public:
    std::string submit_order(const OrderRequest&) override { return ""; }
    bool cancel_order(const std::string&) override { return false; }
    bool modify_order(const std::string&, double, double) override { return false; }
    std::shared_ptr<Order> get_order(const std::string&) const override { return nullptr; }
    std::vector<std::shared_ptr<Order>> get_working_orders() const override { return {}; }
    std::vector<std::shared_ptr<Order>> get_orders_by_symbol(const std::string&) const override { return {}; }
    std::shared_ptr<Position> get_position(const std::string&) const override { return nullptr; }
    std::vector<std::shared_ptr<Position>> get_all_positions() const override { return {}; }
    std::vector<std::shared_ptr<Trade>> get_trades_by_order(const std::string&) const override { return {}; }
    std::vector<std::shared_ptr<Trade>> get_trades_by_symbol(const std::string&) const override { return {}; }
    std::vector<std::shared_ptr<Trade>> get_daily_trades() const override { return {}; }
    void set_order_update_callback(std::function<void(const ExecutionReport&)>) override {}
    void set_trade_callback(std::function<void(const Trade&)>) override {}
    void set_position_update_callback(std::function<void(const Position&)>) override {}
};

} // namespace trading