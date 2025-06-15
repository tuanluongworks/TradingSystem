#ifndef MARKETDATA_H
#define MARKETDATA_H

class MarketData {
public:
    MarketData(double price, double volume);
    
    double getPrice() const;
    double getVolume() const;

private:
    double price;
    double volume;
};

#endif // MARKETDATA_H