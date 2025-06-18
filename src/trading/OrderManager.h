#pragma once
#include "Types.h"
#include <vector>
#include <memory>
#include <mutex>

class DatabaseManager;

class OrderManager {
private:
    std::vector<Order> activeOrders;
    std::shared_ptr<DatabaseManager> dbManager;
    mutable std::mutex ordersMutex;

public:
    explicit OrderManager(std::shared_ptr<DatabaseManager> db);
    ~OrderManager();
    
    std::string createOrder(const Order& order);
    bool cancelOrder(const std::string& orderId);
    bool updateOrder(const std::string& orderId, const Order& updatedOrder);
    std::vector<Order> getActiveOrders() const;
    std::vector<Order> getOrdersByUserId(const std::string& userId) const;
    Order getOrderById(const std::string& orderId) const;
    bool executeOrder(const std::string& orderId);
    
private:
    std::string generateOrderId();
    bool validateOrder(const Order& order) const;
};