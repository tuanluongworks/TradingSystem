#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ui/components/order_entry_panel.hpp"
#include "contracts/ui_interface.hpp"

using namespace trading::ui;
using ::testing::_;
using ::testing::Return;

class MockOrderEntryPanel : public IOrderEntryPanel {
public:
    MOCK_METHOD(void, render, (), (override));
    MOCK_METHOD(void, reset_form, (), (override));
    MOCK_METHOD(void, set_instrument, (const std::string&), (override));
    MOCK_METHOD(OrderFormData, get_form_data, (), (const, override));
    MOCK_METHOD(bool, is_form_valid, (), (const, override));
    MOCK_METHOD(std::string, get_validation_error, (), (const, override));
    MOCK_METHOD(void, set_submit_callback, (std::function<void(const OrderFormData&)>), (override));
    MOCK_METHOD(void, set_clear_callback, (std::function<void()>), (override));
};

class OrderEntryPanelInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        panel = std::make_unique<MockOrderEntryPanel>();
    }

    std::unique_ptr<MockOrderEntryPanel> panel;
};

TEST_F(OrderEntryPanelInterfaceTest, RenderMethodExists) {
    EXPECT_CALL(*panel, render()).Times(1);
    panel->render();
}

TEST_F(OrderEntryPanelInterfaceTest, FormManagementMethodsExist) {
    EXPECT_CALL(*panel, reset_form()).Times(1);
    EXPECT_CALL(*panel, set_instrument("AAPL")).Times(1);

    panel->reset_form();
    panel->set_instrument("AAPL");
}

TEST_F(OrderEntryPanelInterfaceTest, FormStateMethodsExist) {
    OrderFormData test_data;
    test_data.symbol = "AAPL";
    test_data.side = "BUY";
    test_data.type = "MARKET";
    test_data.quantity = 100;
    test_data.price = 0;
    test_data.is_valid = true;

    EXPECT_CALL(*panel, get_form_data())
        .WillOnce(Return(test_data));
    EXPECT_CALL(*panel, is_form_valid())
        .WillOnce(Return(true));
    EXPECT_CALL(*panel, get_validation_error())
        .WillOnce(Return(""));

    auto form_data = panel->get_form_data();
    EXPECT_EQ(form_data.symbol, "AAPL");
    EXPECT_TRUE(panel->is_form_valid());
    EXPECT_EQ(panel->get_validation_error(), "");
}

TEST_F(OrderEntryPanelInterfaceTest, CallbackSettersExist) {
    auto submit_callback = [](const OrderFormData&) {};
    auto clear_callback = []() {};

    EXPECT_CALL(*panel, set_submit_callback(_)).Times(1);
    EXPECT_CALL(*panel, set_clear_callback(_)).Times(1);

    panel->set_submit_callback(submit_callback);
    panel->set_clear_callback(clear_callback);
}