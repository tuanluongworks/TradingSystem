#pragma once
#include "../trading/Types.h"
#include <string>
#include <vector>

class IMarketDataService {
public:
    virtual ~IMarketDataService() = default;
    virtual double getCurrentPrice(const std::string& symbol) const = 0;
    virtual MarketDataPoint getLatestData(const std::string& symbol) const = 0;
    virtual std::vector<MarketDataPoint> getHistoricalData(const std::string& symbol, int limit = 100) const = 0;
    virtual std::vector<std::string> getAvailableSymbols() const = 0;
};
