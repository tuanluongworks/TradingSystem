#ifndef PORTFOLIO_H
#define PORTFOLIO_H

#include <vector>
#include <string>

class Asset {
public:
    std::string name;
    double quantity;
    double purchasePrice;

    Asset(const std::string& name, double quantity, double purchasePrice)
        : name(name), quantity(quantity), purchasePrice(purchasePrice) {}
};

class Portfolio {
private:
    std::vector<Asset> assets;

public:
    void addAsset(const std::string& name, double quantity, double purchasePrice);
    double getPortfolioValue() const;
    const std::vector<Asset>& getAssets() const;
};

#endif // PORTFOLIO_H