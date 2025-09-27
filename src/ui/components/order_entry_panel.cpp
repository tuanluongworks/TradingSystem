#include "order_entry_panel.hpp"
#include <cstring>
#include <algorithm>

namespace trading::ui {

OrderEntryPanel::OrderEntryPanel() {
    reset_form();
}

void OrderEntryPanel::render() {
    if (ImGui::Begin("Order Entry", nullptr, ImGuiWindowFlags_None)) {
        // Title
        ImGui::Text("New Order");
        ImGui::Separator();

        // Symbol selection
        render_symbol_selection();

        // Side selection (BUY/SELL)
        render_side_selection();

        // Type selection (MARKET/LIMIT)
        render_type_selection();

        // Quantity input
        render_quantity_input();

        // Price input (only for LIMIT orders)
        if (type_selection_ == 1) { // LIMIT order
            render_price_input();
        }

        ImGui::Separator();

        // Validation error display
        if (!validation_error_.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", validation_error_.c_str());
        }

        // Action buttons
        render_action_buttons();
    }
    ImGui::End();
}

void OrderEntryPanel::reset_form() {
    strcpy(symbol_buffer_, "AAPL");
    side_selection_ = 0;
    type_selection_ = 0;
    quantity_ = 100.0;
    price_ = 0.0;
    validation_error_.clear();
    validate_form();
}

void OrderEntryPanel::set_instrument(const std::string& symbol) {
    if (symbol.length() < sizeof(symbol_buffer_)) {
        strcpy(symbol_buffer_, symbol.c_str());
        validate_form();
    }
}

OrderFormData OrderEntryPanel::get_form_data() const {
    return form_data_;
}

bool OrderEntryPanel::is_form_valid() const {
    return is_form_valid_;
}

std::string OrderEntryPanel::get_validation_error() const {
    return validation_error_;
}

void OrderEntryPanel::set_submit_callback(std::function<void(const OrderFormData&)> callback) {
    submit_callback_ = std::move(callback);
}

void OrderEntryPanel::set_clear_callback(std::function<void()> callback) {
    clear_callback_ = std::move(callback);
}

void OrderEntryPanel::validate_form() {
    validation_error_.clear();
    is_form_valid_ = true;

    // Validate symbol
    std::string symbol(symbol_buffer_);
    if (symbol.empty()) {
        validation_error_ = "Symbol cannot be empty";
        is_form_valid_ = false;
        return;
    }

    // Validate quantity
    if (quantity_ <= 0.0) {
        validation_error_ = "Quantity must be positive";
        is_form_valid_ = false;
        return;
    }

    // Validate price for LIMIT orders
    if (type_selection_ == 1 && price_ <= 0.0) { // LIMIT order
        validation_error_ = "Price must be positive for limit orders";
        is_form_valid_ = false;
        return;
    }

    // Update form data if valid
    update_form_data();
}

void OrderEntryPanel::update_form_data() {
    form_data_.symbol = std::string(symbol_buffer_);
    form_data_.side = (side_selection_ == 0) ? "BUY" : "SELL";
    form_data_.type = (type_selection_ == 0) ? "MARKET" : "LIMIT";
    form_data_.quantity = quantity_;
    form_data_.price = (type_selection_ == 0) ? 0.0 : price_; // 0 for market orders
    form_data_.is_valid = is_form_valid_;
    form_data_.validation_error = validation_error_;
}

void OrderEntryPanel::render_symbol_selection() {
    ImGui::Text("Symbol:");
    ImGui::SameLine();

    // Combo box for symbol selection
    if (ImGui::BeginCombo("##Symbol", symbol_buffer_)) {
        for (const auto& symbol : available_symbols_) {
            bool is_selected = (symbol == std::string(symbol_buffer_));
            if (ImGui::Selectable(symbol.c_str(), is_selected)) {
                strcpy(symbol_buffer_, symbol.c_str());
                validate_form();
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    // Or allow manual input
    ImGui::SameLine();
    ImGui::Text("or");
    ImGui::SameLine();
    if (ImGui::InputText("##ManualSymbol", symbol_buffer_, sizeof(symbol_buffer_))) {
        validate_form();
    }
}

void OrderEntryPanel::render_side_selection() {
    ImGui::Text("Side:");
    ImGui::SameLine();

    bool side_changed = false;
    if (ImGui::RadioButton("BUY", &side_selection_, 0)) {
        side_changed = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("SELL", &side_selection_, 1)) {
        side_changed = true;
    }

    if (side_changed) {
        validate_form();
    }
}

void OrderEntryPanel::render_type_selection() {
    ImGui::Text("Type:");
    ImGui::SameLine();

    bool type_changed = false;
    if (ImGui::RadioButton("MARKET", &type_selection_, 0)) {
        type_changed = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("LIMIT", &type_selection_, 1)) {
        type_changed = true;
    }

    if (type_changed) {
        validate_form();
    }
}

void OrderEntryPanel::render_quantity_input() {
    ImGui::Text("Quantity:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    if (ImGui::InputDouble("##Quantity", &quantity_, 1.0, 10.0, "%.0f")) {
        validate_form();
    }
}

void OrderEntryPanel::render_price_input() {
    ImGui::Text("Price:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    if (ImGui::InputDouble("##Price", &price_, 0.01, 1.0, "%.2f")) {
        validate_form();
    }
}

void OrderEntryPanel::render_action_buttons() {
    // Submit button
    bool submit_enabled = is_form_valid_;
    if (!submit_enabled) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Submit Order", ImVec2(120, 0))) {
        if (submit_callback_ && is_form_valid_) {
            submit_callback_(form_data_);
        }
    }

    if (!submit_enabled) {
        ImGui::EndDisabled();
    }

    // Clear button
    ImGui::SameLine();
    if (ImGui::Button("Clear", ImVec2(80, 0))) {
        reset_form();
        if (clear_callback_) {
            clear_callback_();
        }
    }

    // Order preview
    if (is_form_valid_) {
        ImGui::Separator();
        ImGui::Text("Order Preview:");
        ImGui::BulletText("%s %.*f %s %s",
                         form_data_.side.c_str(),
                         0, form_data_.quantity,
                         form_data_.symbol.c_str(),
                         form_data_.type.c_str());

        if (form_data_.type == "LIMIT") {
            ImGui::BulletText("At price: %.2f", form_data_.price);
        }

        double notional = form_data_.quantity *
                         (form_data_.type == "MARKET" ? 0.0 : form_data_.price);
        if (notional > 0.0) {
            ImGui::BulletText("Notional: %.2f", notional);
        }
    }
}

} // namespace trading::ui