#pragma once

#include "contracts/trading_engine_api.hpp"

namespace trading {

// Placeholder - will be implemented in Phase 3.4
class MarketDataProvider : public IMarketDataProvider {
public:
    bool connect() override { return false; }
    void disconnect() override {}
    bool is_connected() const override { return false; }
    bool subscribe(const std::string&) override { return false; }
    bool unsubscribe(const std::string&) override { return false; }
    std::vector<std::string> get_subscribed_symbols() const override { return {}; }
    std::shared_ptr<MarketTick> get_latest_tick(const std::string&) const override { return nullptr; }
    std::vector<std::shared_ptr<MarketTick>> get_recent_ticks(const std::string&, int) const override { return {}; }
    void set_tick_callback(std::function<void(const MarketTick&)>) override {}
    void set_connection_callback(std::function<void(bool)>) override {}
};

} // namespace trading