#include <gtest/gtest.h>
#include "OrderManager.h"

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
    // Arrange
    Order order;
    order.id = 1;
    order.symbol = "AAPL";
    order.quantity = 10;
    order.price = 150.0;

    // Act
    bool result = orderManager->createOrder(order);

    // Assert
    EXPECT_TRUE(result);
}

TEST_F(OrderManagerTest, CancelOrder) {
    // Arrange
    Order order;
    order.id = 1;
    order.symbol = "AAPL";
    order.quantity = 10;
    order.price = 150.0;
    orderManager->createOrder(order);

    // Act
    bool result = orderManager->cancelOrder(order.id);

    // Assert
    EXPECT_TRUE(result);
}