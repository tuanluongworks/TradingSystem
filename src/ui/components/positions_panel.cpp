#include "positions_panel.hpp"
#include <iomanip>
#include <sstream>
#include <numeric>

namespace trading::ui {

void PositionsPanel::render() {
    if (ImGui::Begin("Positions", nullptr, ImGuiWindowFlags_None)) {
        // Header controls
        ImGui::Checkbox("Show P&L", &show_pnl_);
        ImGui::SameLine();
        ImGui::Checkbox("Show Unrealized", &show_unrealized_);

        ImGui::Separator();

        // Portfolio summary
        render_summary();

        ImGui::Separator();

        // Positions table
        render_table();
    }
    ImGui::End();
}

void PositionsPanel::update_data(const std::vector<PositionRow>& positions) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    positions_ = positions;
}

void PositionsPanel::clear_data() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    positions_.clear();
    selected_symbol_.clear();
}

void PositionsPanel::set_position_click_callback(std::function<void(const std::string&)> callback) {
    position_click_callback_ = std::move(callback);
}

void PositionsPanel::set_close_position_callback(std::function<void(const std::string&)> callback) {
    close_position_callback_ = std::move(callback);
}

void PositionsPanel::set_show_pnl(bool show) {
    show_pnl_ = show;
}

void PositionsPanel::set_show_unrealized(bool show) {
    show_unrealized_ = show;
}

void PositionsPanel::render_table() {
    std::lock_guard<std::mutex> lock(data_mutex_);

    if (positions_.empty()) {
        ImGui::Text("No positions");
        return;
    }

    // Table setup
    int column_count = 5; // Symbol, Quantity, Avg Price, Current Price, Market Value
    if (show_pnl_) {
        column_count += 2; // Unrealized P&L, Total P&L
    }
    if (show_unrealized_) {
        column_count += 1; // Change %
    }

    const ImGuiTableFlags table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                       ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;

    if (ImGui::BeginTable("##PositionsTable", column_count, table_flags)) {
        // Table headers
        ImGui::TableSetupColumn("Symbol", ImGuiTableColumnFlags_None, 80.0f);
        ImGui::TableSetupColumn("Qty", ImGuiTableColumnFlags_None, 80.0f);
        ImGui::TableSetupColumn("Avg Price", ImGuiTableColumnFlags_None, 80.0f);
        ImGui::TableSetupColumn("Current", ImGuiTableColumnFlags_None, 80.0f);
        ImGui::TableSetupColumn("Market Value", ImGuiTableColumnFlags_None, 100.0f);

        if (show_pnl_) {
            ImGui::TableSetupColumn("Unrealized P&L", ImGuiTableColumnFlags_None, 100.0f);
            ImGui::TableSetupColumn("Total P&L", ImGuiTableColumnFlags_None, 100.0f);
        }

        if (show_unrealized_) {
            ImGui::TableSetupColumn("Change %", ImGuiTableColumnFlags_None, 80.0f);
        }

        ImGui::TableHeadersRow();

        // Table data rows
        for (const auto& position : positions_) {
            ImGui::TableNextRow();

            // Symbol column (clickable)
            ImGui::TableSetColumnIndex(0);
            bool is_selected = (position.symbol == selected_symbol_);
            if (ImGui::Selectable(position.symbol.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns)) {
                selected_symbol_ = position.symbol;
                if (position_click_callback_) {
                    position_click_callback_(position.symbol);
                }
            }

            // Quantity with LONG/SHORT indicator
            ImGui::TableSetColumnIndex(1);
            ImU32 qty_color = (position.quantity > 0) ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 0, 0, 255);
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(qty_color),
                             "%.0f %s", std::abs(position.quantity),
                             position.quantity > 0 ? "L" : "S");

            // Average Price
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.2f", position.average_price);

            // Current Price
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.2f", position.current_price);

            // Market Value
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%s", format_currency(position.market_value).c_str());

            int col_index = 5;

            if (show_pnl_) {
                // Unrealized P&L
                ImGui::TableSetColumnIndex(col_index++);
                ImU32 unrealized_color = get_pnl_color(position.unrealized_pnl);
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(unrealized_color),
                                 "%s", format_currency(position.unrealized_pnl).c_str());

                // Total P&L
                ImGui::TableSetColumnIndex(col_index++);
                ImU32 total_color = get_pnl_color(position.total_pnl);
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(total_color),
                                 "%s", format_currency(position.total_pnl).c_str());
            }

            if (show_unrealized_) {
                // Change %
                ImGui::TableSetColumnIndex(col_index++);
                ImU32 change_color = get_pnl_color(position.change_percent);
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(change_color),
                                 "%s", format_percentage(position.change_percent).c_str());
            }

            // Context menu for position actions
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Close Position")) {
                    if (close_position_callback_) {
                        close_position_callback_(position.symbol);
                    }
                }
                ImGui::MenuItem("View Details", nullptr, false, false); // Disabled for now
                ImGui::EndPopup();
            }
        }

        ImGui::EndTable();
    }
}

void PositionsPanel::render_summary() {
    std::lock_guard<std::mutex> lock(data_mutex_);

    if (positions_.empty()) {
        ImGui::Text("Portfolio Summary: No positions");
        return;
    }

    // Calculate portfolio totals
    double total_market_value = 0.0;
    double total_unrealized_pnl = 0.0;
    double total_realized_pnl = 0.0;
    double total_pnl = 0.0;

    for (const auto& position : positions_) {
        total_market_value += position.market_value;
        total_unrealized_pnl += position.unrealized_pnl;
        total_realized_pnl += position.realized_pnl;
        total_pnl += position.total_pnl;
    }

    // Display summary
    ImGui::Text("Portfolio Summary:");
    ImGui::SameLine();
    ImGui::Text("Positions: %zu", positions_.size());

    ImGui::Text("Market Value: %s", format_currency(total_market_value).c_str());
    ImGui::SameLine(200);

    if (show_pnl_) {
        ImU32 unrealized_color = get_pnl_color(total_unrealized_pnl);
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(unrealized_color),
                         "Unrealized: %s", format_currency(total_unrealized_pnl).c_str());
        ImGui::SameLine(350);

        ImU32 total_color = get_pnl_color(total_pnl);
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(total_color),
                         "Total P&L: %s", format_currency(total_pnl).c_str());
    }
}

ImU32 PositionsPanel::get_pnl_color(double pnl) const {
    if (pnl > 0.0) {
        return IM_COL32(0, 255, 0, 255); // Green for profit
    } else if (pnl < 0.0) {
        return IM_COL32(255, 0, 0, 255); // Red for loss
    } else {
        return IM_COL32(255, 255, 255, 255); // White for break-even
    }
}

std::string PositionsPanel::format_currency(double value) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);

    if (value >= 0) {
        oss << "$" << value;
    } else {
        oss << "-$" << std::abs(value);
    }

    return oss.str();
}

std::string PositionsPanel::format_percentage(double percentage) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << std::showpos << percentage << "%";
    return oss.str();
}

} // namespace trading::ui