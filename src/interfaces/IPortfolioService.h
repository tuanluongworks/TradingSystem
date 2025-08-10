#pragma once
#include "../trading/Types.h"
#include <string>
#include <vector>

class IPortfolioService {
public:
    virtual ~IPortfolioService() = default;
    virtual const std::vector<Asset>& getAssets() const = 0;
    virtual double getTotalValue() const = 0;
    virtual Asset getAsset(const std::string& symbol) const = 0;
    virtual bool hasAsset(const std::string& symbol) const = 0;
};
