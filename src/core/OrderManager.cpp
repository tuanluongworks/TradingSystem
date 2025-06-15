#include "OrderManager.h"
#include "Order.h"
#include <vector>
#include <algorithm>

class OrderManager {
public:
    void createOrder(const Order& order) {
        orders.push_back(order);
    }

    void cancelOrder(int orderId) {
        orders.erase(std::remove_if(orders.begin(), orders.end(),
            [orderId](const Order& order) { return order.getId() == orderId; }), orders.end());
    }

    const std::vector<Order>& getOrders() const {
        return orders;
    }

private:
    std::vector<Order> orders;
};