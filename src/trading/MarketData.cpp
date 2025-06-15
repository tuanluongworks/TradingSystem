#include "MarketData.h"
#include <iostream>
#include <vector>

class MarketData {
public:
    MarketData() {
        // Constructor implementation
    }

    void fetchMarketPrices() {
        // Implementation for fetching market prices
        std::cout << "Fetching market prices..." << std::endl;
        // Simulate fetching data
        marketPrices = {100.5, 101.2, 99.8}; // Example prices
    }

    const std::vector<double>& getMarketPrices() const {
        return marketPrices;
    }

private:
    std::vector<double> marketPrices;
};