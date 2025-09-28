#include "trades_panel.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cstring>

namespace trading::ui {

void TradesPanel::render() {
    if (ImGui::Begin("Trades", nullptr, ImGuiWindowFlags_None)) {
        // Controls
        render_controls();

        ImGui::Separator();

        // Trade summary
        render_trade_summary();

        ImGui::Separator();

        // Trades table
        render_table();
    }
    ImGui::End();
}

void TradesPanel::update_data(const std::vector<TradeRow>& trades) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    trades_ = trades;
    apply_filters();
    sort_trades();
}

void TradesPanel::clear_data() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    trades_.clear();
    filtered_trades_.clear();
}

void TradesPanel::set_show_today_only(bool show_today) {
    show_today_only_ = show_today;
    apply_filters();
}

void TradesPanel::set_auto_scroll(bool auto_scroll) {
    auto_scroll_ = auto_scroll;
}

void TradesPanel::set_max_displayed_trades(int max_trades) {
    max_displayed_trades_ = std::max(10, max_trades);
}

void TradesPanel::render_controls() {
    // Filter controls
    ImGui::Checkbox("Today Only", &show_today_only_);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Show only trades from today");
    }

    ImGui::SameLine();
    ImGui::Checkbox("Auto Scroll", &auto_scroll_);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Automatically scroll to newest trades");
    }

    ImGui::SameLine();
    ImGui::Checkbox("Sort by Time ↓", &sort_by_time_desc_);

    // Symbol filter
    ImGui::Text("Filter by Symbol:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    if (ImGui::InputText("##SymbolFilter", symbol_filter_buffer_, sizeof(symbol_filter_buffer_))) {
        symbol_filter_ = std::string(symbol_filter_buffer_);
        apply_filters();
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear Filter")) {
        symbol_filter_.clear();
        memset(symbol_filter_buffer_, 0, sizeof(symbol_filter_buffer_));
        apply_filters();
    }

    // Max trades control
    ImGui::SameLine();
    ImGui::Text("Max:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    if (ImGui::InputInt("##MaxTrades", &max_displayed_trades_, 10, 100)) {
        max_displayed_trades_ = std::max(10, max_displayed_trades_);
    }
}

void TradesPanel::render_table() {
    std::lock_guard<std::mutex> lock(data_mutex_);

    if (filtered_trades_.empty()) {
        ImGui::Text("No trades to display");
        return;
    }

    // Limit display count
    size_t display_count = std::min(static_cast<size_t>(max_displayed_trades_), filtered_trades_.size());

    // Table setup
    const ImGuiTableFlags table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                       ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;

    if (ImGui::BeginTable("##TradesTable", 7, table_flags)) {
        // Table headers
        ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_None, 120.0f);
        ImGui::TableSetupColumn("Symbol", ImGuiTableColumnFlags_None, 80.0f);
        ImGui::TableSetupColumn("Side", ImGuiTableColumnFlags_None, 60.0f);
        ImGui::TableSetupColumn("Quantity", ImGuiTableColumnFlags_None, 80.0f);
        ImGui::TableSetupColumn("Price", ImGuiTableColumnFlags_None, 80.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_None, 100.0f);
        ImGui::TableSetupColumn("Order ID", ImGuiTableColumnFlags_None, 100.0f);
        ImGui::TableHeadersRow();

        // Table data rows
        for (size_t i = 0; i < display_count; ++i) {
            const auto& trade = filtered_trades_[i];
            ImGui::TableNextRow();

            // Time
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", format_time(trade.execution_time).c_str());

            // Symbol
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", trade.symbol.c_str());

            // Side
            ImGui::TableSetColumnIndex(2);
            ImU32 side_color = get_side_color(trade.side);
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(side_color), "%s", trade.side.c_str());

            // Quantity
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.0f", trade.quantity);

            // Price
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%.2f", trade.price);

            // Value
            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%s", format_currency(trade.notional_value).c_str());

            // Order ID (truncated)
            ImGui::TableSetColumnIndex(6);
            std::string short_order_id = trade.order_id.length() > 8 ?
                                       trade.order_id.substr(0, 8) + "..." :
                                       trade.order_id;
            ImGui::Text("%s", short_order_id.c_str());

            // Tooltip with full order ID
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Full Order ID: %s\nTrade ID: %s",
                                trade.order_id.c_str(),
                                trade.trade_id.c_str());
            }
        }

        // Auto-scroll to bottom if enabled
        if (auto_scroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndTable();
    }

    // Display count info
    if (filtered_trades_.size() > display_count) {
        ImGui::Text("Showing %zu of %zu trades", display_count, filtered_trades_.size());
    } else {
        ImGui::Text("Total: %zu trades", filtered_trades_.size());
    }
}

void TradesPanel::render_trade_summary() {
    std::lock_guard<std::mutex> lock(data_mutex_);

    if (filtered_trades_.empty()) {
        ImGui::Text("Trade Summary: No trades");
        return;
    }

    // Calculate summary statistics
    double total_volume = 0.0;
    double total_value = 0.0;
    size_t buy_count = 0;
    size_t sell_count = 0;

    for (const auto& trade : filtered_trades_) {
        total_volume += trade.quantity;
        total_value += trade.notional_value;
        if (trade.side == "BUY") {
            buy_count++;
        } else {
            sell_count++;
        }
    }

    double avg_trade_size = filtered_trades_.empty() ? 0.0 : total_volume / filtered_trades_.size();

    // Display summary
    ImGui::Text("Summary:");
    ImGui::SameLine();
    ImGui::Text("Trades: %zu (%zu BUY, %zu SELL)", filtered_trades_.size(), buy_count, sell_count);

    ImGui::Text("Volume: %.0f shares", total_volume);
    ImGui::SameLine(200);
    ImGui::Text("Value: %s", format_currency(total_value).c_str());
    ImGui::SameLine(350);
    ImGui::Text("Avg Size: %.0f", avg_trade_size);
}

void TradesPanel::apply_filters() {
    filtered_trades_.clear();

    auto now = std::chrono::system_clock::now();
    auto today_start = std::chrono::time_point_cast<std::chrono::days>(now);

    for (const auto& trade : trades_) {
        // Today filter
        if (show_today_only_) {
            if (trade.execution_time < today_start) {
                continue;
            }
        }

        // Symbol filter
        if (!symbol_filter_.empty()) {
            if (trade.symbol.find(symbol_filter_) == std::string::npos) {
                continue;
            }
        }

        filtered_trades_.push_back(trade);
    }

    sort_trades();
}

void TradesPanel::sort_trades() {
    std::sort(filtered_trades_.begin(), filtered_trades_.end(),
              [this](const TradeRow& a, const TradeRow& b) {
                  if (sort_by_time_desc_) {
                      return a.execution_time > b.execution_time; // Newest first
                  } else {
                      return a.execution_time < b.execution_time; // Oldest first
                  }
              });
}

std::string TradesPanel::format_time(const std::chrono::system_clock::time_point& time) const {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string TradesPanel::format_currency(double value) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);

    if (value >= 0) {
        oss << "$" << value;
    } else {
        oss << "-$" << std::abs(value);
    }

    return oss.str();
}

ImU32 TradesPanel::get_side_color(const std::string& side) const {
    if (side == "BUY") {
        return IM_COL32(0, 255, 0, 255); // Green for BUY
    } else if (side == "SELL") {
        return IM_COL32(255, 0, 0, 255); // Red for SELL
    } else {
        return IM_COL32(255, 255, 255, 255); // White for unknown
    }
}

} // namespace trading::ui