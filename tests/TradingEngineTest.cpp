#include <gtest/gtest.h>
#include "core/TradingEngine.h"

class TradingEngineTest : public ::testing::Test {
protected:
    TradingEngine* tradingEngine;

    void SetUp() override {
        tradingEngine = new TradingEngine();
    }

    void TearDown() override {
        delete tradingEngine;
    }
};

TEST_F(TradingEngineTest, StartTrading) {
    EXPECT_NO_THROW(tradingEngine->startTrading());
}

TEST_F(TradingEngineTest, StopTrading) {
    tradingEngine->startTrading();
    EXPECT_NO_THROW(tradingEngine->stopTrading());
}

TEST_F(TradingEngineTest, ExecuteOrder) {
    tradingEngine->startTrading();
    Order order; // Assuming Order is properly defined in Order.h
    order.setId(1);
    order.setType(OrderType::BUY); // Assuming OrderType is an enum defined somewhere
    order.setQuantity(10);
    
    EXPECT_NO_THROW(tradingEngine->executeOrder(order));
    tradingEngine->stopTrading();
}