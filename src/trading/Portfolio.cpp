#include "Portfolio.h"
#include "DatabaseManager.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>

Portfolio::Portfolio(const std::string& userId, std::shared_ptr<DatabaseManager> db)
    : totalValue(0.0), userId(userId), dbManager(db) {
    if (dbManager && dbManager->isConnected()) {
        loadFromDatabase();
    }
}

Portfolio::~Portfolio() = default;

void Portfolio::addAsset(const Asset& asset) {
    std::lock_guard<std::mutex> lock(portfolioMutex);
    
    auto it = std::find_if(assets.begin(), assets.end(),
        [&asset](const Asset& existing) { return existing.symbol == asset.symbol; });
    
    if (it != assets.end()) {
        // Update existing asset
        double totalCost = (it->quantity * it->averageCost) + (asset.quantity * asset.currentPrice);
        double totalQuantity = it->quantity + asset.quantity;
        
        it->quantity = totalQuantity;
        it->averageCost = totalCost / totalQuantity;
        it->currentPrice = asset.currentPrice;
    } else {
        // Add new asset
        assets.push_back(asset);
    }
    
    calculateTotalValue();
    saveToDatabase();
    
    std::cout << "Asset added to portfolio: " << asset.symbol 
              << " (Quantity: " << asset.quantity << ")\n";
}

bool Portfolio::removeAsset(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(portfolioMutex);
    
    auto it = std::find_if(assets.begin(), assets.end(),
        [&symbol](const Asset& asset) { return asset.symbol == symbol; });
    
    if (it != assets.end()) {
        assets.erase(it);
        calculateTotalValue();
        saveToDatabase();
        
        std::cout << "Asset removed from portfolio: " << symbol << "\n";
        return true;
    }
    
    return false;
}

const std::vector<Asset>& Portfolio::getAssets() const {
    std::lock_guard<std::mutex> lock(portfolioMutex);
    return assets;
}

double Portfolio::getTotalValue() const {
    std::lock_guard<std::mutex> lock(portfolioMutex);
    return totalValue;
}

void Portfolio::updateAssetPrice(const std::string& symbol, double newPrice) {
    std::lock_guard<std::mutex> lock(portfolioMutex);
    
    auto it = std::find_if(assets.begin(), assets.end(),
        [&symbol](Asset& asset) { return asset.symbol == symbol; });
    
    if (it != assets.end()) {
        it->currentPrice = newPrice;
        calculateTotalValue();
        
        std::cout << "Asset price updated: " << symbol << " -> $" << newPrice << "\n";
    }
}

Asset Portfolio::getAsset(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(portfolioMutex);
    
    auto it = std::find_if(assets.begin(), assets.end(),
        [&symbol](const Asset& asset) { return asset.symbol == symbol; });
    
    if (it != assets.end()) {
        return *it;
    }
    
    throw std::runtime_error("Asset not found: " + symbol);
}

bool Portfolio::hasAsset(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(portfolioMutex);
    
    return std::any_of(assets.begin(), assets.end(),
        [&symbol](const Asset& asset) { return asset.symbol == symbol; });
}

void Portfolio::loadFromDatabase() {
    if (dbManager && dbManager->isConnected()) {
        assets = dbManager->findAssetsByUserId(userId);
        calculateTotalValue();
        std::cout << "Portfolio loaded from database for user: " << userId << "\n";
    }
}

void Portfolio::calculateTotalValue() {
    totalValue = 0.0;
    for (const auto& asset : assets) {
        totalValue += asset.quantity * asset.currentPrice;
    }
}

void Portfolio::saveToDatabase() {
    if (dbManager && dbManager->isConnected()) {
        for (const auto& asset : assets) {
            dbManager->save(userId, asset);
        }
    }
}

void Portfolio::onTradeExecution(const TradeExecutionEvent& exec) {
    if (exec.order.userId != userId) return; // ignore other users
    std::lock_guard<std::mutex> lock(portfolioMutex);
    auto it = std::find_if(assets.begin(), assets.end(), [&](const Asset& a){ return a.symbol == exec.order.symbol; });
    double signedQty = (exec.order.type==OrderType::BUY? exec.executedQuantity : -exec.executedQuantity);
    if (it == assets.end()) {
        if (signedQty > 0) { assets.push_back(Asset(exec.order.symbol, exec.executedQuantity, exec.executedPrice)); }
    } else {
        double newQty = it->quantity + signedQty;
        if (newQty <= 0) { assets.erase(it); }
        else {
            if (signedQty > 0) { double totalCost = it->quantity * it->averageCost + exec.executedQuantity * exec.executedPrice; it->quantity = newQty; it->averageCost = totalCost / it->quantity; it->currentPrice = exec.executedPrice; }
            else { it->quantity = newQty; it->currentPrice = exec.executedPrice; }
        }
    }
    calculateTotalValue(); saveToDatabase();
}