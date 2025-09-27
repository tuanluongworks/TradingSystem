#include "market_data_panel.hpp"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace trading::ui {

void MarketDataPanel::render() {
    if (ImGui::Begin("Market Data", nullptr, ImGuiWindowFlags_None)) {
        // Header controls
        if (ImGui::Button("Add Symbol")) {
            show_add_symbol_ = true;
        }
        ImGui::SameLine();

        if (ImGui::Button("Clear All")) {
            clear_data();
        }
        ImGui::SameLine();

        ImGui::Checkbox("Auto Sort", &auto_sort_);
        ImGui::SameLine();

        ImGui::SetNextItemWidth(80);
        ImGui::SliderInt("Precision", &precision_, 0, 6, "%d");

        ImGui::Separator();

        // Render the market data table
        render_table();

        // Render add symbol popup
        if (show_add_symbol_) {
            render_add_symbol_popup();
        }
    }
    ImGui::End();
}

void MarketDataPanel::update_data(const std::vector<MarketDataRow>& data) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    market_data_ = data;

    if (auto_sort_) {
        sort_data();
    }
}

void MarketDataPanel::clear_data() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    market_data_.clear();
}

void MarketDataPanel::set_symbol_click_callback(std::function<void(const std::string&)> callback) {
    symbol_click_callback_ = std::move(callback);
}

void MarketDataPanel::set_subscribe_callback(std::function<void(const std::string&)> callback) {
    subscribe_callback_ = std::move(callback);
}

void MarketDataPanel::set_auto_sort(bool enabled) {
    auto_sort_ = enabled;
    if (enabled) {
        sort_data();
    }
}

void MarketDataPanel::set_precision(int decimal_places) {
    precision_ = std::max(0, std::min(6, decimal_places));
}

void MarketDataPanel::sort_data() {
    std::sort(market_data_.begin(), market_data_.end(), [this](const MarketDataRow& a, const MarketDataRow& b) {
        bool result = false;
        switch (sort_column_) {
            case 0: // Symbol
                result = a.symbol < b.symbol;
                break;
            case 1: // Last Price
                result = a.last_price < b.last_price;
                break;
            case 2: // Bid
                result = a.bid_price < b.bid_price;
                break;
            case 3: // Ask
                result = a.ask_price < b.ask_price;
                break;
            case 4: // Spread
                result = a.spread < b.spread;
                break;
            case 5: // Change %
                result = a.change_percent < b.change_percent;
                break;
            default:
                result = a.symbol < b.symbol;
                break;
        }
        return sort_ascending_ ? result : !result;
    });
}

void MarketDataPanel::render_table() {
    std::lock_guard<std::mutex> lock(data_mutex_);

    if (market_data_.empty()) {
        ImGui::Text("No market data available");
        return;
    }

    // Table setup
    const ImGuiTableFlags table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                       ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable |
                                       ImGuiTableFlags_ScrollY;

    if (ImGui::BeginTable("##MarketDataTable", 6, table_flags)) {
        // Table headers
        ImGui::TableSetupColumn("Symbol", ImGuiTableColumnFlags_DefaultSort, 80.0f, 0);
        ImGui::TableSetupColumn("Last", ImGuiTableColumnFlags_None, 80.0f, 1);
        ImGui::TableSetupColumn("Bid", ImGuiTableColumnFlags_None, 80.0f, 2);
        ImGui::TableSetupColumn("Ask", ImGuiTableColumnFlags_None, 80.0f, 3);
        ImGui::TableSetupColumn("Spread", ImGuiTableColumnFlags_None, 60.0f, 4);
        ImGui::TableSetupColumn("Change %", ImGuiTableColumnFlags_None, 80.0f, 5);
        ImGui::TableHeadersRow();

        // Handle sorting
        ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs();
        if (sort_specs != nullptr && sort_specs->SpecsDirty) {
            if (sort_specs->SpecsCount == 1) {
                sort_column_ = sort_specs->Specs[0].ColumnUserID;
                sort_ascending_ = sort_specs->Specs[0].SortDirection == ImGuiSortDirection_Ascending;
                sort_data();
            }
            sort_specs->SpecsDirty = false;
        }

        // Table data rows
        for (const auto& row : market_data_) {
            ImGui::TableNextRow();

            // Symbol column (clickable)
            ImGui::TableSetColumnIndex(0);
            if (ImGui::Selectable(row.symbol.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) {
                if (symbol_click_callback_) {
                    symbol_click_callback_(row.symbol);
                }
            }

            // Last Price
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, row.is_stale ? 0.5f : 1.0f),
                             "%.*f", precision_, row.last_price);

            // Bid Price
            ImGui::TableSetColumnIndex(2);
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, row.is_stale ? 0.5f : 1.0f),
                             "%.*f", precision_, row.bid_price);

            // Ask Price
            ImGui::TableSetColumnIndex(3);
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, row.is_stale ? 0.5f : 1.0f),
                             "%.*f", precision_, row.ask_price);

            // Spread
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%.*f", precision_, row.spread);

            // Change %
            ImGui::TableSetColumnIndex(5);
            ImU32 change_color = get_price_color(row.change_percent);
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(change_color),
                             "%+.2f%%", row.change_percent);
        }

        ImGui::EndTable();
    }
}

void MarketDataPanel::render_add_symbol_popup() {
    if (ImGui::BeginPopupModal("Add Symbol", &show_add_symbol_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter symbol to subscribe:");
        ImGui::SetNextItemWidth(200);

        bool enter_pressed = ImGui::InputText("##Symbol", new_symbol_buffer_,
                                            sizeof(new_symbol_buffer_),
                                            ImGuiInputTextFlags_EnterReturnsTrue);

        ImGui::Separator();

        if ((ImGui::Button("Subscribe") || enter_pressed) && strlen(new_symbol_buffer_) > 0) {
            if (subscribe_callback_) {
                subscribe_callback_(std::string(new_symbol_buffer_));
            }
            memset(new_symbol_buffer_, 0, sizeof(new_symbol_buffer_));
            show_add_symbol_ = false;
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            memset(new_symbol_buffer_, 0, sizeof(new_symbol_buffer_));
            show_add_symbol_ = false;
        }

        ImGui::EndPopup();
    }

    // Show the popup
    if (show_add_symbol_) {
        ImGui::OpenPopup("Add Symbol");
    }
}

ImU32 MarketDataPanel::get_price_color(double change_percent) const {
    if (change_percent > 0.0) {
        // Green for positive changes
        return IM_COL32(0, 255, 0, 255);
    } else if (change_percent < 0.0) {
        // Red for negative changes
        return IM_COL32(255, 0, 0, 255);
    } else {
        // White for no change
        return IM_COL32(255, 255, 255, 255);
    }
}

} // namespace trading::ui