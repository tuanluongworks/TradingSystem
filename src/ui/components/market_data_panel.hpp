#pragma once

#include <vector>
#include <string>
#include <functional>
#include <algorithm>
#include <mutex>
#include <imgui.h>

// Need to include the contract from the specs directory
#include "../../../specs/001-c-trading-system/contracts/ui_interface.hpp"

namespace trading::ui {

class MarketDataPanel : public IMarketDataPanel {
private:
    std::vector<MarketDataRow> market_data_;
    std::function<void(const std::string&)> symbol_click_callback_;
    std::function<void(const std::string&)> subscribe_callback_;
    bool auto_sort_ = true;
    int precision_ = 2;
    mutable std::mutex data_mutex_;

    // UI state
    bool show_add_symbol_ = false;
    char new_symbol_buffer_[16] = "";

    // Table sorting
    bool sort_ascending_ = true;
    int sort_column_ = 0; // 0=symbol, 1=last_price, 2=bid, 3=ask, 4=spread, 5=change%

    void sort_data();
    void render_table();
    void render_add_symbol_popup();
    ImU32 get_price_color(double change_percent) const;

public:
    MarketDataPanel() = default;
    ~MarketDataPanel() override = default;

    // IMarketDataPanel interface
    void render() override;
    void update_data(const std::vector<MarketDataRow>& data) override;
    void clear_data() override;
    void set_symbol_click_callback(std::function<void(const std::string&)> callback) override;
    void set_subscribe_callback(std::function<void(const std::string&)> callback) override;
    void set_auto_sort(bool enabled) override;
    void set_precision(int decimal_places) override;
};

} // namespace trading::ui