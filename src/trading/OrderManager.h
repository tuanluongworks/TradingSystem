#ifndef ORDER_MANAGER_H
#define ORDER_MANAGER_H

#include <string>
#include <vector>

class Order {
public:
    Order(int id, const std::string& symbol, int quantity, double price);
    int getId() const;
    std::string getSymbol() const;
    int getQuantity() const;
    double getPrice() const;

private:
    int id;
    std::string symbol;
    int quantity;
    double price;
};

class OrderManager {
public:
    OrderManager();
    void createOrder(const std::string& symbol, int quantity, double price);
    void cancelOrder(int orderId);
    std::vector<Order> getOrders() const;

private:
    std::vector<Order> orders;
};

#endif // ORDER_MANAGER_H