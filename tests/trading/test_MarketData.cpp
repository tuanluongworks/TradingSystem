#include "../../src/trading/MarketData.h"
#include "../../src/database/DatabaseManager.h"
#include <memory>
#include <thread>
#include <chrono>

TEST(MarketData_UpdateAndGetPrice) {
    auto dbManager = std::make_shared<DatabaseManager>();
    MarketData marketData(dbManager);
    
    marketData.updatePrice("TSLA", 800.0, 1000000.0);
    
    double price = marketData.getCurrentPrice("TSLA");
    ASSERT_EQ(price, 800.0);
    
    return true;
}

TEST(MarketData_GetLatestData) {
    auto dbManager = std::make_shared<DatabaseManager>();
    MarketData marketData(dbManager);
    
    marketData.updatePrice("AMZN", 3200.0, 500000.0);
    
    MarketDataPoint data = marketData.getLatestData("AMZN");
    ASSERT_EQ(data.symbol, "AMZN");
    ASSERT_EQ(data.price, 3200.0);
    ASSERT_EQ(data.volume, 500000.0);
    
    return true;
}

TEST(MarketData_GetAvailableSymbols) {
    auto dbManager = std::make_shared<DatabaseManager>();
    MarketData marketData(dbManager);
    
    // Default symbols should be available
    auto symbols = marketData.getAvailableSymbols();
    ASSERT_TRUE(symbols.size() >= 5); // At least the default symbols
    
    // Check that AAPL is in the list
    bool hasAAPL = false;
    for (const auto& symbol : symbols) {
        if (symbol == "AAPL") {
            hasAAPL = true;
            break;
        }
    }
    ASSERT_TRUE(hasAAPL);
    
    return true;
}

TEST(MarketData_NonExistentSymbol) {
    auto dbManager = std::make_shared<DatabaseManager>();
    MarketData marketData(dbManager);
    
    try {
        marketData.getCurrentPrice("NONEXISTENT");
        return false; // Should have thrown
    } catch (const std::runtime_error&) {
        return true; // Expected exception
    }
}

TEST(MarketData_Simulation) {
    auto dbManager = std::make_shared<DatabaseManager>();
    MarketData marketData(dbManager);
    
    double initialPrice = marketData.getCurrentPrice("AAPL");
    
    marketData.startSimulation();
    ASSERT_TRUE(marketData.isSimulationRunning());
    
    // Wait a bit for price changes
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    marketData.stopSimulation();
    ASSERT_FALSE(marketData.isSimulationRunning());
    
    // Price might have changed (but not guaranteed in such a short time)
    double finalPrice = marketData.getCurrentPrice("AAPL");
    
    // Just verify the price is still valid
    ASSERT_TRUE(finalPrice > 0);
    
    return true;
} 