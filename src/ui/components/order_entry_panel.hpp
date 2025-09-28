#pragma once

#include <string>
#include <functional>
#include <vector>
#include <imgui.h>

// Include the contract from the include directory
#include "contracts/ui_interface.hpp"

namespace trading::ui {

class OrderEntryPanel : public IOrderEntryPanel {
private:
    // Form data
    OrderFormData form_data_;

    // UI state
    char symbol_buffer_[16] = "AAPL";
    int side_selection_ = 0;     // 0=BUY, 1=SELL
    int type_selection_ = 0;     // 0=MARKET, 1=LIMIT
    double quantity_ = 100.0;
    double price_ = 0.0;

    // Validation
    std::string validation_error_;
    bool is_form_valid_ = false;

    // Callbacks
    std::function<void(const OrderFormData&)> submit_callback_;
    std::function<void()> clear_callback_;

    // Available symbols (could be populated from market data)
    std::vector<std::string> available_symbols_ = {"AAPL", "GOOGL", "MSFT", "TSLA", "AMZN"};

    // Helper methods
    void validate_form();
    void update_form_data();
    void render_symbol_selection();
    void render_side_selection();
    void render_type_selection();
    void render_quantity_input();
    void render_price_input();
    void render_action_buttons();

public:
    OrderEntryPanel();
    ~OrderEntryPanel() override = default;

    // IOrderEntryPanel interface
    void render() override;
    void reset_form() override;
    void set_instrument(const std::string& symbol) override;
    OrderFormData get_form_data() const override;
    bool is_form_valid() const override;
    std::string get_validation_error() const override;
    void set_submit_callback(std::function<void(const OrderFormData&)> callback) override;
    void set_clear_callback(std::function<void()> callback) override;
};

} // namespace trading::ui