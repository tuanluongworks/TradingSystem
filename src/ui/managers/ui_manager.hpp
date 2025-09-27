#pragma once

#include "contracts/ui_interface.hpp"

namespace trading::ui {

// Placeholder - will be implemented in Phase 3.6
class UIManager : public IUIManager {
public:
    // Implementation will be added later
    bool initialize() override { return false; }
    void run() override {}
    void shutdown() override {}
    void show_market_data_window(bool) override {}
    void show_order_entry_window(bool) override {}
    void show_positions_window(bool) override {}
    void show_trades_window(bool) override {}
    void show_status_window(bool) override {}
    void update_market_data(const std::vector<MarketDataRow>&) override {}
    void update_orders(const std::vector<OrderRow>&) override {}
    void update_positions(const std::vector<PositionRow>&) override {}
    void update_trades(const std::vector<TradeRow>&) override {}
    void update_connection_status(bool, const std::string&) override {}
    void set_order_submit_callback(std::function<void(const OrderFormData&)>) override {}
    void set_order_cancel_callback(std::function<void(const std::string&)>) override {}
    void set_symbol_subscribe_callback(std::function<void(const std::string&)>) override {}
    void set_symbol_unsubscribe_callback(std::function<void(const std::string&)>) override {}
};

} // namespace trading::ui