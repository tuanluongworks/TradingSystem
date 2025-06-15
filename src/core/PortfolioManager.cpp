#include "PortfolioManager.h"
#include <iostream>
#include <numeric>

PortfolioManager::PortfolioManager() {
    // Initialize an empty portfolio
}

void PortfolioManager::addAsset(const Asset& asset) {
    portfolio.push_back(asset);
}

double PortfolioManager::getPortfolioValue() const {
    return std::accumulate(portfolio.begin(), portfolio.end(), 0.0,
        [](double total, const Asset& asset) {
            return total + asset.getValue();
        });
}

void PortfolioManager::printPortfolio() const {
    for (const auto& asset : portfolio) {
        std::cout << "Asset: " << asset.getName() << ", Value: " << asset.getValue() << std::endl;
    }
}