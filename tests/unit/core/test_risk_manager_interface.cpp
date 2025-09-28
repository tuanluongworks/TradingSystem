#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "core/risk/risk_manager.hpp"
#include "contracts/trading_engine_api.hpp"

using namespace trading;
using ::testing::_;
using ::testing::Return;

class MockRiskManager : public IRiskManager {
public:
    MOCK_METHOD(bool, validate_order, (const OrderRequest&), (const, override));
    MOCK_METHOD(std::string, get_rejection_reason, (const OrderRequest&), (const, override));
    MOCK_METHOD(bool, set_position_limit, (const std::string&, double), (override));
    MOCK_METHOD(bool, set_order_size_limit, (const std::string&, double), (override));
    MOCK_METHOD(bool, set_daily_loss_limit, (double), (override));
    MOCK_METHOD(double, get_position_limit, (const std::string&), (const, override));
    MOCK_METHOD(double, get_order_size_limit, (const std::string&), (const, override));
    MOCK_METHOD(double, get_daily_loss_limit, (), (const, override));
    MOCK_METHOD(double, get_current_exposure, (const std::string&), (const, override));
    MOCK_METHOD(double, get_daily_pnl, (), (const, override));
    MOCK_METHOD(double, get_total_position_value, (), (const, override));
};

class RiskManagerInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        risk_manager = std::make_unique<MockRiskManager>();
    }

    std::unique_ptr<MockRiskManager> risk_manager;
};

TEST_F(RiskManagerInterfaceTest, ValidateOrderMethodExists) {
    OrderRequest request;
    request.instrument_symbol = "AAPL";
    request.side = OrderSide::BUY;
    request.type = OrderType::MARKET;
    request.quantity = 100;
    request.price = 0;

    EXPECT_CALL(*risk_manager, validate_order(_))
        .WillOnce(Return(true));

    EXPECT_TRUE(risk_manager->validate_order(request));
}

TEST_F(RiskManagerInterfaceTest, GetRejectionReasonMethodExists) {
    OrderRequest request;
    request.quantity = 1000000; // Excessive quantity

    EXPECT_CALL(*risk_manager, get_rejection_reason(_))
        .WillOnce(Return("Order size exceeds limit"));

    std::string reason = risk_manager->get_rejection_reason(request);
    EXPECT_EQ(reason, "Order size exceeds limit");
}

TEST_F(RiskManagerInterfaceTest, LimitManagementMethodsExist) {
    EXPECT_CALL(*risk_manager, set_position_limit("AAPL", 10000))
        .WillOnce(Return(true));
    EXPECT_CALL(*risk_manager, set_order_size_limit("AAPL", 1000))
        .WillOnce(Return(true));
    EXPECT_CALL(*risk_manager, set_daily_loss_limit(50000))
        .WillOnce(Return(true));

    EXPECT_TRUE(risk_manager->set_position_limit("AAPL", 10000));
    EXPECT_TRUE(risk_manager->set_order_size_limit("AAPL", 1000));
    EXPECT_TRUE(risk_manager->set_daily_loss_limit(50000));
}

TEST_F(RiskManagerInterfaceTest, LimitQueryMethodsExist) {
    EXPECT_CALL(*risk_manager, get_position_limit("AAPL"))
        .WillOnce(Return(10000));
    EXPECT_CALL(*risk_manager, get_order_size_limit("AAPL"))
        .WillOnce(Return(1000));
    EXPECT_CALL(*risk_manager, get_daily_loss_limit())
        .WillOnce(Return(50000));

    EXPECT_EQ(risk_manager->get_position_limit("AAPL"), 10000);
    EXPECT_EQ(risk_manager->get_order_size_limit("AAPL"), 1000);
    EXPECT_EQ(risk_manager->get_daily_loss_limit(), 50000);
}

TEST_F(RiskManagerInterfaceTest, RiskMetricsMethodsExist) {
    EXPECT_CALL(*risk_manager, get_current_exposure("AAPL"))
        .WillOnce(Return(500000));
    EXPECT_CALL(*risk_manager, get_daily_pnl())
        .WillOnce(Return(2500));
    EXPECT_CALL(*risk_manager, get_total_position_value())
        .WillOnce(Return(1500000));

    EXPECT_EQ(risk_manager->get_current_exposure("AAPL"), 500000);
    EXPECT_EQ(risk_manager->get_daily_pnl(), 2500);
    EXPECT_EQ(risk_manager->get_total_position_value(), 1500000);
}