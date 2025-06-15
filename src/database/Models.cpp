#include "Models.h"

// Implementation of data models used in the application

// Example model for a trading order
class Order {
public:
    int id;
    std::string symbol;
    double price;
    int quantity;
    std::string side; // "buy" or "sell"

    Order(int id, const std::string& symbol, double price, int quantity, const std::string& side)
        : id(id), symbol(symbol), price(price), quantity(quantity), side(side) {}
};

// Example model for a user portfolio
class Portfolio {
public:
    int userId;
    std::vector<Order> orders;

    Portfolio(int userId) : userId(userId) {}

    void addOrder(const Order& order) {
        orders.push_back(order);
    }
};