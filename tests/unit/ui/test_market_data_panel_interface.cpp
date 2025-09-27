#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ui/components/market_data_panel.hpp"
#include "contracts/ui_interface.hpp"

using namespace trading::ui;
using ::testing::_;

class MockMarketDataPanel : public IMarketDataPanel {
public:
    MOCK_METHOD(void, render, (), (override));
    MOCK_METHOD(void, update_data, (const std::vector<MarketDataRow>&), (override));
    MOCK_METHOD(void, clear_data, (), (override));
    MOCK_METHOD(void, set_symbol_click_callback, (std::function<void(const std::string&)>), (override));
    MOCK_METHOD(void, set_subscribe_callback, (std::function<void(const std::string&)>), (override));
    MOCK_METHOD(void, set_auto_sort, (bool), (override));
    MOCK_METHOD(void, set_precision, (int), (override));
};

class MarketDataPanelInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        panel = std::make_unique<MockMarketDataPanel>();
    }

    std::unique_ptr<MockMarketDataPanel> panel;
};

TEST_F(MarketDataPanelInterfaceTest, RenderMethodExists) {
    EXPECT_CALL(*panel, render()).Times(1);
    panel->render();
}

TEST_F(MarketDataPanelInterfaceTest, UpdateDataMethodExists) {
    std::vector<MarketDataRow> test_data;
    EXPECT_CALL(*panel, update_data(_)).Times(1);
    panel->update_data(test_data);
}

TEST_F(MarketDataPanelInterfaceTest, ClearDataMethodExists) {
    EXPECT_CALL(*panel, clear_data()).Times(1);
    panel->clear_data();
}

TEST_F(MarketDataPanelInterfaceTest, CallbackSettersExist) {
    auto symbol_click_callback = [](const std::string&) {};
    auto subscribe_callback = [](const std::string&) {};

    EXPECT_CALL(*panel, set_symbol_click_callback(_)).Times(1);
    EXPECT_CALL(*panel, set_subscribe_callback(_)).Times(1);

    panel->set_symbol_click_callback(symbol_click_callback);
    panel->set_subscribe_callback(subscribe_callback);
}

TEST_F(MarketDataPanelInterfaceTest, ConfigurationMethodsExist) {
    EXPECT_CALL(*panel, set_auto_sort(true)).Times(1);
    EXPECT_CALL(*panel, set_precision(4)).Times(1);

    panel->set_auto_sort(true);
    panel->set_precision(4);
}