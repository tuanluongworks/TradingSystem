#include "TradingEngine.h"
#include "OrderManager.h"
#include "PortfolioManager.h"
#include "MarketData.h"
#include "Logger.h"

TradingEngine::TradingEngine()
    : orderManager(new OrderManager()), portfolioManager(new PortfolioManager()) {
}

TradingEngine::~TradingEngine() {
    delete orderManager;
    delete portfolioManager;
}

void TradingEngine::startTrading() {
    Logger::logInfo("Trading engine started.");
    // Initialize market data and strategies here
    // Start the trading loop
}

void TradingEngine::stopTrading() {
    Logger::logInfo("Trading engine stopped.");
    // Clean up resources and stop trading activities
}

void TradingEngine::executeOrder(const Order& order) {
    // Logic to execute the order
    Logger::logInfo("Executing order: " + std::to_string(order.getId()));
    orderManager->createOrder(order);
}

void TradingEngine::updatePortfolio(const Trade& trade) {
    // Logic to update the portfolio based on the trade
    portfolioManager->addAsset(trade.getAsset(), trade.getQuantity());
}