#ifndef TRADINGCONTROLLER_H
#define TRADINGCONTROLLER_H

#include <string>
#include "../interfaces/IOrderService.h"
#include "../interfaces/IPortfolioService.h"
#include "../interfaces/IMarketDataService.h"

class TradingController {
public:
    TradingController(IOrderService* orderService, IPortfolioService* portfolioService, IMarketDataService* marketDataService);
    std::string createOrder(const std::string& orderDetails);
    std::string cancelOrder(const std::string& orderId);
    std::string getPortfolio();
    std::string getMarketData();
private:
    IOrderService* orderService;
    IPortfolioService* portfolioService;
    IMarketDataService* marketDataService;
};

#endif // TRADINGCONTROLLER_H