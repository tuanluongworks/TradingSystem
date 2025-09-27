#pragma once

#include "contracts/ui_interface.hpp"

namespace trading::ui {

// Placeholder - will be implemented in Phase 3.6
class PositionsPanel : public IPositionsPanel {
public:
    void render() override {}
    void update_data(const std::vector<PositionRow>&) override {}
    void clear_data() override {}
    void set_position_click_callback(std::function<void(const std::string&)>) override {}
    void set_close_position_callback(std::function<void(const std::string&)>) override {}
    void set_show_pnl(bool) override {}
    void set_show_unrealized(bool) override {}
};

} // namespace trading::ui