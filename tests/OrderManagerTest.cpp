#include <gtest/gtest.h>
#include "core/OrderManager.h"
#include "models/Order.h"

class OrderManagerTest : public ::testing::Test {
protected:
    OrderManager* orderManager;

    void SetUp() override {
        orderManager = new OrderManager();
    }

    void TearDown() override {
        delete orderManager;
    }
};

TEST_F(OrderManagerTest, CreateOrder) {
    Order order = orderManager->createOrder("Buy", 100, 10.5);
    EXPECT_EQ(order.getType(), "Buy");
    EXPECT_EQ(order.getQuantity(), 100);
    EXPECT_EQ(order.getPrice(), 10.5);
}

TEST_F(OrderManagerTest, CancelOrder) {
    Order order = orderManager->createOrder("Sell", 50, 20.0);
    EXPECT_TRUE(orderManager->cancelOrder(order.getId()));
    EXPECT_FALSE(orderManager->cancelOrder(order.getId())); // Canceling again should fail
}