#pragma once
#include "Types.h"
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include "../interfaces/IPortfolioService.h"

class DatabaseManager;

class Portfolio : public IPortfolioService {
private:
    std::vector<Asset> assets;
    double totalValue;
    std::string userId;
    std::shared_ptr<DatabaseManager> dbManager;
    mutable std::mutex portfolioMutex;

public:
    explicit Portfolio(const std::string& userId, std::shared_ptr<DatabaseManager> db = nullptr);
    ~Portfolio();
    
    void addAsset(const Asset& asset);
    bool removeAsset(const std::string& symbol);
    const std::vector<Asset>& getAssets() const override;
    double getTotalValue() const override;
    void updateAssetPrice(const std::string& symbol, double newPrice);
    Asset getAsset(const std::string& symbol) const override;
    bool hasAsset(const std::string& symbol) const override;
    void loadFromDatabase();
    
private:
    void calculateTotalValue();
    void saveToDatabase();
};