#ifndef MARKETDATAPROVIDER_H
#define MARKETDATAPROVIDER_H

#include "DataProvider.h"
#include "MarketData.h"

class MarketDataProvider : public DataProvider {
public:
    MarketDataProvider();
    ~MarketDataProvider();

    MarketData fetchMarketData(const std::string& symbol) override;
    void subscribeToMarketData(const std::string& symbol);
    void unsubscribeFromMarketData(const std::string& symbol);

private:
    void updateMarketData(const std::string& symbol);
};

#endif // MARKETDATAPROVIDER_H