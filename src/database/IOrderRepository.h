#pragma once
#include "../trading/Types.h"
#include <vector>
#include <string>
#include <optional>

// Persistence abstraction for Orders.
class IOrderRepository {
public:
    virtual ~IOrderRepository() = default;
    virtual bool save(const Order& order) = 0;
    virtual std::optional<Order> findOrderById(const std::string& orderId) = 0;
    virtual std::vector<Order> findOrdersByUserId(const std::string& userId) = 0;
    virtual bool updateStatus(const std::string& orderId, OrderStatus status) = 0;
};
