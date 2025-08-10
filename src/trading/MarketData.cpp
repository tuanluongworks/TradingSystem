#include "MarketData.h"
#include "DatabaseManager.h"
#include "OrderEvents.h"
#include "../infrastructure/LockFreeQueue.h"
#include <random>
#include <thread>
#include <chrono>
#include <iostream>
#include <algorithm>

extern SPSCQueue<TradingEvent>* g_orderEventQueue;

MarketData::MarketData(std::shared_ptr<DatabaseManager> db)
    : dbManager(db), isSimulating(false) {
    
    // Initialize with some default symbols and prices
    updatePrice("AAPL", 150.0);
    updatePrice("GOOGL", 2800.0);
    updatePrice("MSFT", 300.0);
    updatePrice("TSLA", 800.0);
    updatePrice("AMZN", 3200.0);
    
    std::cout << "MarketData initialized with default symbols\n";
}

MarketData::~MarketData() {
    stopSimulation();
}

void MarketData::updatePrice(const std::string& symbol, double price, double volume) {
    std::lock_guard<std::mutex> lock(dataMutex);
    
    MarketDataPoint point{symbol, price, volume};
    latestPrices[symbol] = point;
    historicalData.push_back(point);
    
    saveToDatabase(point);
    
    if (g_orderEventQueue) { g_orderEventQueue->push(TradingEvent{MarketDataUpdateEvent{point}}); }
    
    std::cout << "Price updated: " << symbol << " -> $" << price << "\n";
}

double MarketData::getCurrentPrice(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(dataMutex);
    
    auto it = latestPrices.find(symbol);
    if (it != latestPrices.end()) {
        return it->second.price;
    }
    
    throw std::runtime_error("Symbol not found: " + symbol);
}

MarketDataPoint MarketData::getLatestData(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(dataMutex);
    
    auto it = latestPrices.find(symbol);
    if (it != latestPrices.end()) {
        return it->second;
    }
    
    throw std::runtime_error("Symbol not found: " + symbol);
}

std::vector<MarketDataPoint> MarketData::getHistoricalData(const std::string& symbol, int limit) const {
    std::lock_guard<std::mutex> lock(dataMutex);
    
    std::vector<MarketDataPoint> result;
    
    for (auto it = historicalData.rbegin(); it != historicalData.rend() && result.size() < limit; ++it) {
        if (it->symbol == symbol) {
            result.push_back(*it);
        }
    }
    
    return result;
}

std::vector<std::string> MarketData::getAvailableSymbols() const {
    std::lock_guard<std::mutex> lock(dataMutex);
    
    std::vector<std::string> symbols;
    for (const auto& pair : latestPrices) {
        symbols.push_back(pair.first);
    }
    
    return symbols;
}

void MarketData::startSimulation() {
    if (!isSimulating) {
        isSimulating = true;
        std::thread(&MarketData::simulateMarketData, this).detach();
        std::cout << "Market data simulation started\n";
    }
}

void MarketData::stopSimulation() {
    isSimulating = false;
    std::cout << "Market data simulation stopped\n";
}

bool MarketData::isSimulationRunning() const {
    return isSimulating;
}

void MarketData::simulateMarketData() {
    while (isSimulating) {
        {
            std::lock_guard<std::mutex> lock(dataMutex);
            for (auto& pair : latestPrices) {
                double newPrice = generateRandomPrice(pair.first, pair.second.price);
                pair.second.price = newPrice;
                pair.second.timestamp = std::chrono::system_clock::now();
                
                historicalData.push_back(pair.second);
            }
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

double MarketData::generateRandomPrice(const std::string& symbol, double currentPrice) const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(-0.02, 0.02); // +/- 2% change
    
    double change = dis(gen);
    double newPrice = currentPrice * (1.0 + change);
    
    // Ensure price doesn't go below $1
    return std::max(newPrice, 1.0);
}

void MarketData::saveToDatabase(const MarketDataPoint& data) {
    if (dbManager && dbManager->isConnected()) {
        dbManager->saveMarketData(data);
    }
}