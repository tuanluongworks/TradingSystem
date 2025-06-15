#include <iostream>
#include "core/TradingEngine.h"

int main() {
    TradingEngine tradingEngine;

    std::cout << "Initializing Trading System..." << std::endl;
    tradingEngine.startTrading();

    // Main trading loop
    while (true) {
        // Here you would typically fetch market data, execute strategies, etc.
        // For demonstration purposes, we'll just break the loop after one iteration.
        break;
    }

    tradingEngine.stopTrading();
    std::cout << "Trading System stopped." << std::endl;

    return 0;
}