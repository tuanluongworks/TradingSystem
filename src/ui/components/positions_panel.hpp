#pragma once

#include <vector>
#include <string>
#include <functional>
#include <mutex>
#include <imgui.h>

// Include the contract from the include directory
#include "contracts/ui_interface.hpp"

namespace trading::ui {

class PositionsPanel : public IPositionsPanel {
private:
    std::vector<PositionRow> positions_;
    std::function<void(const std::string&)> position_click_callback_;
    std::function<void(const std::string&)> close_position_callback_;
    mutable std::mutex data_mutex_;

    // Display options
    bool show_pnl_ = true;
    bool show_unrealized_ = true;

    // UI state
    std::string selected_symbol_;

    // Helper methods
    void render_table();
    void render_summary();
    ImU32 get_pnl_color(double pnl) const;
    std::string format_currency(double value) const;
    std::string format_percentage(double percentage) const;

public:
    PositionsPanel() = default;
    ~PositionsPanel() override = default;

    // IPositionsPanel interface
    void render() override;
    void update_data(const std::vector<PositionRow>& positions) override;
    void clear_data() override;
    void set_position_click_callback(std::function<void(const std::string&)> callback) override;
    void set_close_position_callback(std::function<void(const std::string&)> callback) override;
    void set_show_pnl(bool show) override;
    void set_show_unrealized(bool show) override;
};

} // namespace trading::ui