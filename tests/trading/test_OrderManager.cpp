#include "../../src/trading/OrderManager.h"
#include "../../src/database/DatabaseManager.h"
#include <memory>

TEST(OrderManager_CreateOrder) {
    auto dbManager = std::make_shared<DatabaseManager>();
    OrderManager orderManager(dbManager);
    
    Order order;
    order.symbol = "AAPL";
    order.type = OrderType::BUY;
    order.quantity = 10;
    order.price = 150.0;
    order.userId = "test_user";
    
    std::string orderId = orderManager.createOrder(order);
    
    ASSERT_TRUE(!orderId.empty());
    ASSERT_TRUE(orderId.substr(0, 3) == "ORD");
    
    return true;
}

TEST(OrderManager_CancelOrder) {
    auto dbManager = std::make_shared<DatabaseManager>();
    OrderManager orderManager(dbManager);
    
    Order order;
    order.symbol = "AAPL";
    order.type = OrderType::BUY;
    order.quantity = 10;
    order.price = 150.0;
    order.userId = "test_user";
    
    std::string orderId = orderManager.createOrder(order);
    ASSERT_TRUE(!orderId.empty());
    
    bool result = orderManager.cancelOrder(orderId);
    ASSERT_TRUE(result);
    
    // Try to cancel again - should fail
    bool result2 = orderManager.cancelOrder(orderId);
    ASSERT_FALSE(result2);
    
    return true;
}

TEST(OrderManager_GetOrderById) {
    auto dbManager = std::make_shared<DatabaseManager>();
    OrderManager orderManager(dbManager);
    
    Order order;
    order.symbol = "GOOGL";
    order.type = OrderType::SELL;
    order.quantity = 5;
    order.price = 2800.0;
    order.userId = "test_user";
    
    std::string orderId = orderManager.createOrder(order);
    
    try {
        Order retrievedOrder = orderManager.getOrderById(orderId);
        ASSERT_EQ(retrievedOrder.id, orderId);
        ASSERT_EQ(retrievedOrder.symbol, order.symbol);
        ASSERT_EQ(retrievedOrder.quantity, order.quantity);
        return true;
    } catch (...) {
        return false;
    }
}

TEST(OrderManager_InvalidOrder) {
    auto dbManager = std::make_shared<DatabaseManager>();
    OrderManager orderManager(dbManager);
    
    Order order;
    order.symbol = ""; // Invalid - empty symbol
    order.type = OrderType::BUY;
    order.quantity = 10;
    order.price = 150.0;
    order.userId = "test_user";
    
    try {
        orderManager.createOrder(order);
        return false; // Should have thrown
    } catch (const std::invalid_argument&) {
        return true; // Expected exception
    }
}