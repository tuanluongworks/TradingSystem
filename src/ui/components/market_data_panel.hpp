#pragma once

#include "contracts/ui_interface.hpp"

namespace trading::ui {

// Placeholder - will be implemented in Phase 3.6
class MarketDataPanel : public IMarketDataPanel {
public:
    void render() override {}
    void update_data(const std::vector<MarketDataRow>&) override {}
    void clear_data() override {}
    void set_symbol_click_callback(std::function<void(const std::string&)>) override {}
    void set_subscribe_callback(std::function<void(const std::string&)>) override {}
    void set_auto_sort(bool) override {}
    void set_precision(int) override {}
};

} // namespace trading::ui