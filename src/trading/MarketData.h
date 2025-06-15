#ifndef MARKETDATA_H
#define MARKETDATA_H

#include <string>
#include <vector>

class MarketData {
public:
    MarketData();
    ~MarketData();

    void fetchMarketPrices();
    std::vector<double> getPrices() const;

private:
    std::vector<double> prices;
};

#endif // MARKETDATA_H