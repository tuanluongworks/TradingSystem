#include "../../src/trading/Portfolio.h"
#include "../../src/database/DatabaseManager.h"
#include <memory>

TEST(Portfolio_AddAsset) {
    auto dbManager = std::make_shared<DatabaseManager>();
    Portfolio portfolio("test_user", dbManager);
    
    Asset asset("AAPL", 10, 150.0);
    portfolio.addAsset(asset);
    
    ASSERT_TRUE(portfolio.hasAsset("AAPL"));
    
    Asset retrievedAsset = portfolio.getAsset("AAPL");
    ASSERT_EQ(retrievedAsset.symbol, "AAPL");
    ASSERT_EQ(retrievedAsset.quantity, 10.0);
    
    return true;
}

TEST(Portfolio_RemoveAsset) {
    auto dbManager = std::make_shared<DatabaseManager>();
    Portfolio portfolio("test_user", dbManager);
    
    Asset asset("MSFT", 5, 300.0);
    portfolio.addAsset(asset);
    
    ASSERT_TRUE(portfolio.hasAsset("MSFT"));
    
    bool removed = portfolio.removeAsset("MSFT");
    ASSERT_TRUE(removed);
    ASSERT_FALSE(portfolio.hasAsset("MSFT"));
    
    return true;
}

TEST(Portfolio_UpdateAssetPrice) {
    auto dbManager = std::make_shared<DatabaseManager>();
    Portfolio portfolio("test_user", dbManager);
    
    Asset asset("GOOGL", 2, 2800.0);
    portfolio.addAsset(asset);
    
    portfolio.updateAssetPrice("GOOGL", 2900.0);
    
    Asset updated = portfolio.getAsset("GOOGL");
    ASSERT_EQ(updated.currentPrice, 2900.0);
    
    return true;
}

TEST(Portfolio_TotalValue) {
    auto dbManager = std::make_shared<DatabaseManager>();
    Portfolio portfolio("test_user", dbManager);
    
    portfolio.addAsset(Asset("AAPL", 10, 150.0));  // 1500
    portfolio.addAsset(Asset("MSFT", 5, 300.0));   // 1500
    portfolio.addAsset(Asset("GOOGL", 2, 2800.0)); // 5600
    
    double totalValue = portfolio.getTotalValue();
    ASSERT_EQ(totalValue, 8600.0);
    
    return true;
}

TEST(Portfolio_AddExistingAsset) {
    auto dbManager = std::make_shared<DatabaseManager>();
    Portfolio portfolio("test_user", dbManager);
    
    // Add initial asset
    portfolio.addAsset(Asset("AAPL", 10, 150.0));
    
    // Add more of the same asset
    portfolio.addAsset(Asset("AAPL", 5, 160.0));
    
    Asset combined = portfolio.getAsset("AAPL");
    ASSERT_EQ(combined.quantity, 15.0); // 10 + 5
    
    // Average cost should be weighted average
    double expectedAvgCost = (10 * 150.0 + 5 * 160.0) / 15.0;
    ASSERT_TRUE(std::abs(combined.averageCost - expectedAvgCost) < 0.01);
    
    return true;
} 