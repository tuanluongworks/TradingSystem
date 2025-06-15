#ifndef ORDER_H
#define ORDER_H

#include <string>

class Order {
public:
    enum class OrderType {
        BUY,
        SELL
    };

    Order(int id, OrderType type, double quantity);

    int getId() const;
    OrderType getType() const;
    double getQuantity() const;

private:
    int id;
    OrderType type;
    double quantity;
};

#endif // ORDER_H