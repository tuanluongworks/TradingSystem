#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ui/components/positions_panel.hpp"
#include "contracts/ui_interface.hpp"

using namespace trading::ui;
using ::testing::_;

class MockPositionsPanel : public IPositionsPanel {
public:
    MOCK_METHOD(void, render, (), (override));
    MOCK_METHOD(void, update_data, (const std::vector<PositionRow>&), (override));
    MOCK_METHOD(void, clear_data, (), (override));
    MOCK_METHOD(void, set_position_click_callback, (std::function<void(const std::string&)>), (override));
    MOCK_METHOD(void, set_close_position_callback, (std::function<void(const std::string&)>), (override));
    MOCK_METHOD(void, set_show_pnl, (bool), (override));
    MOCK_METHOD(void, set_show_unrealized, (bool), (override));
};

class PositionsPanelInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        panel = std::make_unique<MockPositionsPanel>();
    }

    std::unique_ptr<MockPositionsPanel> panel;
};

TEST_F(PositionsPanelInterfaceTest, RenderMethodExists) {
    EXPECT_CALL(*panel, render()).Times(1);
    panel->render();
}

TEST_F(PositionsPanelInterfaceTest, DataManagementMethodsExist) {
    std::vector<PositionRow> test_data;
    EXPECT_CALL(*panel, update_data(_)).Times(1);
    EXPECT_CALL(*panel, clear_data()).Times(1);

    panel->update_data(test_data);
    panel->clear_data();
}

TEST_F(PositionsPanelInterfaceTest, CallbackSettersExist) {
    auto position_click_callback = [](const std::string&) {};
    auto close_position_callback = [](const std::string&) {};

    EXPECT_CALL(*panel, set_position_click_callback(_)).Times(1);
    EXPECT_CALL(*panel, set_close_position_callback(_)).Times(1);

    panel->set_position_click_callback(position_click_callback);
    panel->set_close_position_callback(close_position_callback);
}

TEST_F(PositionsPanelInterfaceTest, DisplayOptionsExist) {
    EXPECT_CALL(*panel, set_show_pnl(true)).Times(1);
    EXPECT_CALL(*panel, set_show_unrealized(false)).Times(1);

    panel->set_show_pnl(true);
    panel->set_show_unrealized(false);
}