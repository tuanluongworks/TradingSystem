class Trade {
public:
    Trade(int tradeId, double price, int quantity);

    int getTradeId() const;
    double getPrice() const;
    int getQuantity() const;

private:
    int tradeId;
    double price;
    int quantity;
};