#pragma once
#include "../trading/Types.h"
#include <string>
#include <vector>

// Domain service boundary for order operations. Controllers / adapters depend on this
// instead of concrete OrderManager implementation to enable inversion of dependencies.
class IOrderService {
public:
    virtual ~IOrderService() = default;
    virtual std::string createOrder(const Order& order) = 0;
    virtual bool cancelOrder(const std::string& orderId) = 0;
    virtual bool updateOrder(const std::string& orderId, const Order& updatedOrder) = 0;
    virtual std::vector<Order> getActiveOrders() const = 0;
    virtual std::vector<Order> getOrdersByUserId(const std::string& userId) const = 0;
    virtual Order getOrderById(const std::string& orderId) const = 0;
    virtual bool executeOrder(const std::string& orderId) = 0;
};
