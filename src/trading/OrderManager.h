#pragma once
#include "Types.h"
#include <vector>
#include <memory>
#include <mutex>
#include "../interfaces/IOrderService.h"

class DatabaseManager;

class OrderManager : public IOrderService {
private:
    std::vector<Order> activeOrders;
    std::shared_ptr<DatabaseManager> dbManager;
    mutable std::mutex ordersMutex;

public:
    explicit OrderManager(std::shared_ptr<DatabaseManager> db);
    ~OrderManager();
    // IOrderService implementation
    std::string createOrder(const Order& order) override;
    bool cancelOrder(const std::string& orderId) override;
    bool updateOrder(const std::string& orderId, const Order& updatedOrder) override;
    std::vector<Order> getActiveOrders() const override;
    std::vector<Order> getOrdersByUserId(const std::string& userId) const override;
    Order getOrderById(const std::string& orderId) const override;
    bool executeOrder(const std::string& orderId) override;
private:
    std::string generateOrderId();
    bool validateOrder(const Order& order) const;
};