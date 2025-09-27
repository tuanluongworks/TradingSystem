#pragma once

#include "contracts/trading_engine_api.hpp"

namespace trading {

// Placeholder - will be implemented in Phase 3.4
class SQLiteService : public IPersistenceService {
public:
    bool save_trade(const Trade&) override { return false; }
    bool save_order(const Order&) override { return false; }
    bool update_position(const Position&) override { return false; }
    std::vector<std::shared_ptr<Trade>> load_trades_by_date(const std::chrono::system_clock::time_point&) override { return {}; }
    std::vector<std::shared_ptr<Order>> load_orders_by_date(const std::chrono::system_clock::time_point&) override { return {}; }
    std::vector<std::shared_ptr<Position>> load_all_positions() override { return {}; }
    bool backup_to_file(const std::string&) override { return false; }
    bool restore_from_file(const std::string&) override { return false; }
    bool is_available() const override { return false; }
    std::string get_status() const override { return ""; }
};

} // namespace trading