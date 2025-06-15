class OrderManager {
public:
    OrderManager();
    ~OrderManager();

    void createOrder(const Order& order);
    void modifyOrder(int orderId, const Order& updatedOrder);
    void cancelOrder(int orderId);
    std::vector<Order> getActiveOrders() const;

private:
    std::vector<Order> activeOrders;
};