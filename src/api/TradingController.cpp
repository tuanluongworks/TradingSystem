#include "TradingController.h"
#include "../trading/OrderManager.h"
#include "../trading/Portfolio.h"
#include "../trading/MarketData.h"
#include "../utils/Logger.h"
#include "../database/DatabaseManager.h"
#include <sstream>
#include <iomanip>

TradingController::TradingController(IOrderService* orderService, IPortfolioService* portfolioService, IMarketDataService* marketDataService)
    : orderService(orderService), portfolioService(portfolioService), marketDataService(marketDataService) {}

std::string TradingController::createOrder(const std::string& orderDetails) {
        try {
            // TODO: Parse orderDetails JSON to extract order parameters
            // For now, create a sample order
            Order order;
            order.symbol = "AAPL";
            order.type = OrderType::BUY;
            order.quantity = 100;
            order.price = 150.0;
            order.userId = "user123";
            
            std::string orderId = orderService->createOrder(order);
            
            std::ostringstream response;
            response << R"({"success": true, "orderId": ")" << orderId << R"(", "message": "Order created successfully"})";
            return response.str();
            
        } catch (const std::exception& e) {
            std::ostringstream response;
            response << R"({"success": false, "error": ")" << e.what() << R"("})";
            return response.str();
        }
    }

std::string TradingController::cancelOrder(const std::string& orderId) {
        try {
            bool success = orderService->cancelOrder(orderId);
            
            std::ostringstream response;
            if (success) {
                response << R"({"success": true, "message": "Order cancelled successfully"})";
            } else {
                response << R"({"success": false, "error": "Order not found or already processed"})";
            }
            return response.str();
            
        } catch (const std::exception& e) {
            std::ostringstream response;
            response << R"({"success": false, "error": ")" << e.what() << R"("})";
            return response.str();
        }
    }

std::string TradingController::getPortfolio() {
        try {
            const auto& assets = portfolioService->getAssets();
            double totalValue = portfolioService->getTotalValue();
            
            std::ostringstream response;
            response << std::fixed << std::setprecision(2);
            response << R"({"success": true, "totalValue": )" << totalValue << R"(, "assets": [)";
            
            for (size_t i = 0; i < assets.size(); ++i) {
                if (i > 0) response << ", ";
                const auto& asset = assets[i];
                response << R"({"symbol": ")" << asset.symbol << R"(", )"
                        << R"("quantity": )" << asset.quantity << R"(, )"
                        << R"("currentPrice": )" << asset.currentPrice << R"(, )"
                        << R"("averageCost": )" << asset.averageCost << R"(})";
            }
            
            response << "]}";
            return response.str();
            
        } catch (const std::exception& e) {
            std::ostringstream response;
            response << R"({"success": false, "error": ")" << e.what() << R"("})";
            return response.str();
        }
    }

std::string TradingController::getMarketData() {
        try {
            auto symbols = marketDataService->getAvailableSymbols();
            
            std::ostringstream response;
            response << std::fixed << std::setprecision(2);
            response << R"({"success": true, "marketData": [)";
            
            for (size_t i = 0; i < symbols.size(); ++i) {
                if (i > 0) response << ", ";
                const auto& symbol = symbols[i];
                auto data = marketDataService->getLatestData(symbol);
                
                response << R"({"symbol": ")" << symbol << R"(", )"
                        << R"("price": )" << data.price << R"(, )"
                        << R"("volume": )" << data.volume << R"(})";
            }
            
            response << "]}";
            return response.str();
            
        } catch (const std::exception& e) {
            std::ostringstream response;
            response << R"({"success": false, "error": ")" << e.what() << R"("})";
            return response.str();
        }
    }