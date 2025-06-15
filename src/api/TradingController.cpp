#include "TradingController.h"
#include "../trading/OrderManager.h"
#include "../trading/Portfolio.h"
#include "../utils/Logger.h"
#include "../database/DatabaseManager.h"

class TradingController {
public:
    TradingController(OrderManager* orderManager, Portfolio* portfolio, DatabaseManager* dbManager)
        : orderManager(orderManager), portfolio(portfolio), dbManager(dbManager) {}

    void createOrder(const std::string& orderDetails) {
        // Logic to create an order
        orderManager->createOrder(orderDetails);
        Logger::log("Order created: " + orderDetails);
    }

    void cancelOrder(int orderId) {
        // Logic to cancel an order
        orderManager->cancelOrder(orderId);
        Logger::log("Order canceled: " + std::to_string(orderId));
    }

    void getPortfolio() {
        // Logic to retrieve portfolio
        portfolio->getAssets();
        Logger::log("Portfolio retrieved.");
    }

private:
    OrderManager* orderManager;
    Portfolio* portfolio;
    DatabaseManager* dbManager;
};