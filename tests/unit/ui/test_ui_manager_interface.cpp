#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ui/managers/ui_manager.hpp"
#include "contracts/ui_interface.hpp"

using namespace trading::ui;
using ::testing::_;
using ::testing::Return;

// Mock implementation of UI Manager for contract testing
class MockUIManager : public IUIManager {
public:
    MOCK_METHOD(bool, initialize, (), (override));
    MOCK_METHOD(void, run, (), (override));
    MOCK_METHOD(void, shutdown, (), (override));

    MOCK_METHOD(void, show_market_data_window, (bool), (override));
    MOCK_METHOD(void, show_order_entry_window, (bool), (override));
    MOCK_METHOD(void, show_positions_window, (bool), (override));
    MOCK_METHOD(void, show_trades_window, (bool), (override));
    MOCK_METHOD(void, show_status_window, (bool), (override));

    MOCK_METHOD(void, update_market_data, (const std::vector<MarketDataRow>&), (override));
    MOCK_METHOD(void, update_orders, (const std::vector<OrderRow>&), (override));
    MOCK_METHOD(void, update_positions, (const std::vector<PositionRow>&), (override));
    MOCK_METHOD(void, update_trades, (const std::vector<TradeRow>&), (override));
    MOCK_METHOD(void, update_connection_status, (bool, const std::string&), (override));

    MOCK_METHOD(void, set_order_submit_callback, (std::function<void(const OrderFormData&)>), (override));
    MOCK_METHOD(void, set_order_cancel_callback, (std::function<void(const std::string&)>), (override));
    MOCK_METHOD(void, set_symbol_subscribe_callback, (std::function<void(const std::string&)>), (override));
    MOCK_METHOD(void, set_symbol_unsubscribe_callback, (std::function<void(const std::string&)>), (override));
};

class UIManagerInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        ui_manager = std::make_unique<MockUIManager>();
    }

    std::unique_ptr<MockUIManager> ui_manager;
};

TEST_F(UIManagerInterfaceTest, InitializeReturnsSuccess) {
    EXPECT_CALL(*ui_manager, initialize())
        .WillOnce(Return(true));

    EXPECT_TRUE(ui_manager->initialize());
}

TEST_F(UIManagerInterfaceTest, ShowWindowMethodsExist) {
    EXPECT_CALL(*ui_manager, show_market_data_window(true)).Times(1);
    EXPECT_CALL(*ui_manager, show_order_entry_window(false)).Times(1);
    EXPECT_CALL(*ui_manager, show_positions_window(true)).Times(1);
    EXPECT_CALL(*ui_manager, show_trades_window(true)).Times(1);
    EXPECT_CALL(*ui_manager, show_status_window(false)).Times(1);

    ui_manager->show_market_data_window(true);
    ui_manager->show_order_entry_window(false);
    ui_manager->show_positions_window(true);
    ui_manager->show_trades_window(true);
    ui_manager->show_status_window(false);
}

TEST_F(UIManagerInterfaceTest, UpdateDataMethodsExist) {
    std::vector<MarketDataRow> market_data;
    std::vector<OrderRow> orders;
    std::vector<PositionRow> positions;
    std::vector<TradeRow> trades;

    EXPECT_CALL(*ui_manager, update_market_data(_)).Times(1);
    EXPECT_CALL(*ui_manager, update_orders(_)).Times(1);
    EXPECT_CALL(*ui_manager, update_positions(_)).Times(1);
    EXPECT_CALL(*ui_manager, update_trades(_)).Times(1);
    EXPECT_CALL(*ui_manager, update_connection_status(true, "Connected")).Times(1);

    ui_manager->update_market_data(market_data);
    ui_manager->update_orders(orders);
    ui_manager->update_positions(positions);
    ui_manager->update_trades(trades);
    ui_manager->update_connection_status(true, "Connected");
}

TEST_F(UIManagerInterfaceTest, CallbackSettersExist) {
    auto order_submit_callback = [](const OrderFormData&) {};
    auto order_cancel_callback = [](const std::string&) {};
    auto symbol_subscribe_callback = [](const std::string&) {};
    auto symbol_unsubscribe_callback = [](const std::string&) {};

    EXPECT_CALL(*ui_manager, set_order_submit_callback(_)).Times(1);
    EXPECT_CALL(*ui_manager, set_order_cancel_callback(_)).Times(1);
    EXPECT_CALL(*ui_manager, set_symbol_subscribe_callback(_)).Times(1);
    EXPECT_CALL(*ui_manager, set_symbol_unsubscribe_callback(_)).Times(1);

    ui_manager->set_order_submit_callback(order_submit_callback);
    ui_manager->set_order_cancel_callback(order_cancel_callback);
    ui_manager->set_symbol_subscribe_callback(symbol_subscribe_callback);
    ui_manager->set_symbol_unsubscribe_callback(symbol_unsubscribe_callback);
}

TEST_F(UIManagerInterfaceTest, RunAndShutdownMethodsExist) {
    EXPECT_CALL(*ui_manager, run()).Times(1);
    EXPECT_CALL(*ui_manager, shutdown()).Times(1);

    ui_manager->run();
    ui_manager->shutdown();
}