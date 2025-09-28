#pragma once

#include <vector>
#include <string>
#include <functional>
#include <mutex>
#include <imgui.h>

// Include the contract from the include directory
#include "contracts/ui_interface.hpp"

namespace trading::ui {

class TradesPanel {
private:
    std::vector<TradeRow> trades_;
    mutable std::mutex data_mutex_;

    // Display options
    bool show_today_only_ = false;
    bool auto_scroll_ = true;
    int max_displayed_trades_ = 1000;

    // Filtering
    std::string symbol_filter_;
    char symbol_filter_buffer_[16] = "";

    // Sorting
    bool sort_by_time_desc_ = true;

    // Helper methods
    void render_controls();
    void render_table();
    void render_trade_summary();
    void apply_filters();
    void sort_trades();
    std::string format_time(const std::chrono::system_clock::time_point& time) const;
    std::string format_currency(double value) const;
    ImU32 get_side_color(const std::string& side) const;

    // Filtered and sorted data
    std::vector<TradeRow> filtered_trades_;

public:
    TradesPanel() = default;
    ~TradesPanel() = default;

    // Main interface
    void render();
    void update_data(const std::vector<TradeRow>& trades);
    void clear_data();

    // Configuration
    void set_show_today_only(bool show_today);
    void set_auto_scroll(bool auto_scroll);
    void set_max_displayed_trades(int max_trades);
};

} // namespace trading::ui