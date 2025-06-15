#ifndef TRADINGCONTROLLER_H
#define TRADINGCONTROLLER_H

#include <string>
#include "OrderManager.h"
#include "Portfolio.h"
#include "MarketData.h"

class TradingController {
public:
    TradingController(OrderManager* orderManager, Portfolio* portfolio, MarketData* marketData);
    
    std::string createOrder(const std::string& orderDetails);
    std::string cancelOrder(const std::string& orderId);
    std::string getPortfolio();
    std::string getMarketData();

private:
    OrderManager* orderManager;
    Portfolio* portfolio;
    MarketData* marketData;
};

#endif // TRADINGCONTROLLER_H