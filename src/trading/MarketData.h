#pragma once
#include "Types.h"
#include <vector>
#include <memory>
#include <mutex>
#include <map>
#include "../interfaces/IMarketDataService.h"

class DatabaseManager;

class MarketData : public IMarketDataService {
private:
    std::map<std::string, MarketDataPoint> latestPrices;
    std::vector<MarketDataPoint> historicalData;
    std::shared_ptr<DatabaseManager> dbManager;
    mutable std::mutex dataMutex;
    bool isSimulating;

public:
    explicit MarketData(std::shared_ptr<DatabaseManager> db = nullptr);
    ~MarketData();
    
    void updatePrice(const std::string& symbol, double price, double volume = 0.0);
    double getCurrentPrice(const std::string& symbol) const override;
    MarketDataPoint getLatestData(const std::string& symbol) const override;
    std::vector<MarketDataPoint> getHistoricalData(const std::string& symbol, int limit = 100) const override;
    std::vector<std::string> getAvailableSymbols() const override;
    
    void startSimulation();
    void stopSimulation();
    bool isSimulationRunning() const;
    
private:
    void simulateMarketData();
    double generateRandomPrice(const std::string& symbol, double currentPrice) const;
    void saveToDatabase(const MarketDataPoint& data);
};