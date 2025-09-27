#pragma once

#include "contracts/ui_interface.hpp"

namespace trading::ui {

// Placeholder - will be implemented in Phase 3.6
class OrderEntryPanel : public IOrderEntryPanel {
public:
    void render() override {}
    void reset_form() override {}
    void set_instrument(const std::string&) override {}
    OrderFormData get_form_data() const override { return {}; }
    bool is_form_valid() const override { return false; }
    std::string get_validation_error() const override { return ""; }
    void set_submit_callback(std::function<void(const OrderFormData&)>) override {}
    void set_clear_callback(std::function<void()>) override {}
};

} // namespace trading::ui