#pragma once
#include "../trading/Types.h"
#include <string>

class IMarketDataRepository {
public:
    virtual ~IMarketDataRepository() = default;
    virtual bool save(const MarketDataPoint& data) = 0;
    virtual MarketDataPoint latest(const std::string& symbol) = 0;
};
