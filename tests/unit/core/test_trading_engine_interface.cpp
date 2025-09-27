#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "core/engine/trading_engine.hpp"
#include "contracts/trading_engine_api.hpp"

using namespace trading;
using ::testing::_;
using ::testing::Return;

class MockTradingEngine : public ITradingEngine {
public:
    MOCK_METHOD(std::string, submit_order, (const OrderRequest&), (override));
    MOCK_METHOD(bool, cancel_order, (const std::string&), (override));
    MOCK_METHOD(bool, modify_order, (const std::string&, double, double), (override));
    MOCK_METHOD(std::shared_ptr<Order>, get_order, (const std::string&), (const, override));
    MOCK_METHOD(std::vector<std::shared_ptr<Order>>, get_working_orders, (), (const, override));
    MOCK_METHOD(std::vector<std::shared_ptr<Order>>, get_orders_by_symbol, (const std::string&), (const, override));
    MOCK_METHOD(std::shared_ptr<Position>, get_position, (const std::string&), (const, override));
    MOCK_METHOD(std::vector<std::shared_ptr<Position>>, get_all_positions, (), (const, override));
    MOCK_METHOD(std::vector<std::shared_ptr<Trade>>, get_trades_by_order, (const std::string&), (const, override));
    MOCK_METHOD(std::vector<std::shared_ptr<Trade>>, get_trades_by_symbol, (const std::string&), (const, override));
    MOCK_METHOD(std::vector<std::shared_ptr<Trade>>, get_daily_trades, (), (const, override));
    MOCK_METHOD(void, set_order_update_callback, (std::function<void(const ExecutionReport&)>), (override));
    MOCK_METHOD(void, set_trade_callback, (std::function<void(const Trade&)>), (override));
    MOCK_METHOD(void, set_position_update_callback, (std::function<void(const Position&)>), (override));
};

class TradingEngineInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine = std::make_unique<MockTradingEngine>();
    }

    std::unique_ptr<MockTradingEngine> engine;
};

TEST_F(TradingEngineInterfaceTest, SubmitOrderReturnsOrderId) {
    OrderRequest request;
    request.instrument_symbol = "AAPL";
    request.side = OrderSide::BUY;
    request.type = OrderType::MARKET;
    request.quantity = 100;
    request.price = 0;

    EXPECT_CALL(*engine, submit_order(_))
        .WillOnce(Return("ORDER_123"));

    std::string order_id = engine->submit_order(request);
    EXPECT_EQ(order_id, "ORDER_123");
}

TEST_F(TradingEngineInterfaceTest, CancelOrderReturnsSuccess) {
    EXPECT_CALL(*engine, cancel_order("ORDER_123"))
        .WillOnce(Return(true));

    EXPECT_TRUE(engine->cancel_order("ORDER_123"));
}

TEST_F(TradingEngineInterfaceTest, ModifyOrderReturnsSuccess) {
    EXPECT_CALL(*engine, modify_order("ORDER_123", 200.0, 150.50))
        .WillOnce(Return(true));

    EXPECT_TRUE(engine->modify_order("ORDER_123", 200.0, 150.50));
}

TEST_F(TradingEngineInterfaceTest, QueryMethodsExist) {
    EXPECT_CALL(*engine, get_order("ORDER_123"))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*engine, get_working_orders())
        .WillOnce(Return(std::vector<std::shared_ptr<Order>>{}));
    EXPECT_CALL(*engine, get_orders_by_symbol("AAPL"))
        .WillOnce(Return(std::vector<std::shared_ptr<Order>>{}));

    auto order = engine->get_order("ORDER_123");
    auto working_orders = engine->get_working_orders();
    auto symbol_orders = engine->get_orders_by_symbol("AAPL");

    EXPECT_EQ(order, nullptr);
    EXPECT_TRUE(working_orders.empty());
    EXPECT_TRUE(symbol_orders.empty());
}

TEST_F(TradingEngineInterfaceTest, PositionQueryMethodsExist) {
    EXPECT_CALL(*engine, get_position("AAPL"))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*engine, get_all_positions())
        .WillOnce(Return(std::vector<std::shared_ptr<Position>>{}));

    auto position = engine->get_position("AAPL");
    auto all_positions = engine->get_all_positions();

    EXPECT_EQ(position, nullptr);
    EXPECT_TRUE(all_positions.empty());
}

TEST_F(TradingEngineInterfaceTest, TradeQueryMethodsExist) {
    EXPECT_CALL(*engine, get_trades_by_order("ORDER_123"))
        .WillOnce(Return(std::vector<std::shared_ptr<Trade>>{}));
    EXPECT_CALL(*engine, get_trades_by_symbol("AAPL"))
        .WillOnce(Return(std::vector<std::shared_ptr<Trade>>{}));
    EXPECT_CALL(*engine, get_daily_trades())
        .WillOnce(Return(std::vector<std::shared_ptr<Trade>>{}));

    auto order_trades = engine->get_trades_by_order("ORDER_123");
    auto symbol_trades = engine->get_trades_by_symbol("AAPL");
    auto daily_trades = engine->get_daily_trades();

    EXPECT_TRUE(order_trades.empty());
    EXPECT_TRUE(symbol_trades.empty());
    EXPECT_TRUE(daily_trades.empty());
}

TEST_F(TradingEngineInterfaceTest, CallbackSettersExist) {
    auto order_callback = [](const ExecutionReport&) {};
    auto trade_callback = [](const Trade&) {};
    auto position_callback = [](const Position&) {};

    EXPECT_CALL(*engine, set_order_update_callback(_)).Times(1);
    EXPECT_CALL(*engine, set_trade_callback(_)).Times(1);
    EXPECT_CALL(*engine, set_position_update_callback(_)).Times(1);

    engine->set_order_update_callback(order_callback);
    engine->set_trade_callback(trade_callback);
    engine->set_position_update_callback(position_callback);
}