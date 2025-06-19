#include <iostream>
#include <memory>
#include <signal.h>
#include "server/HttpServer.h"
#include "server/Router.h"
#include "api/TradingController.h"
#include "api/AuthController.h"
#include "trading/OrderManager.h"
#include "trading/Portfolio.h"
#include "trading/MarketData.h"
#include "database/DatabaseManager.h"
#include "utils/Config.h"
#include "utils/Logger.h"
#include "common/Constants.h"

// Global server pointer for signal handling
std::unique_ptr<HttpServer> g_server;

void signalHandler(int signum) {
    std::cout << "\nInterrupt signal (" << signum << ") received.\n";
    if (g_server) {
        g_server->stop();
    }
    exit(signum);
}

int main(int argc, char* argv[]) {
    // Set up signal handling
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        // Load configuration
        std::string configFile = "config/development.ini";
        if (argc > 1) {
            configFile = argv[1];
        }
        
        Config config(configFile);
        
        // Initialize logger
        Logger logger("trading_system.log");
        
        // Initialize database
        auto dbManager = std::make_shared<DatabaseManager>();
        if (!dbManager->connect()) {
            std::cerr << "Failed to connect to database" << std::endl;
            return 1;
        }
        
        // Initialize trading components
        auto orderManager = std::make_shared<OrderManager>(dbManager);
        auto marketData = std::make_shared<MarketData>(dbManager);
        auto portfolio = std::make_shared<Portfolio>("default_user", dbManager);
        
        // Start market data simulation
        marketData->startSimulation();
        
        // Initialize controllers
        auto tradingController = std::make_shared<TradingController>(
            orderManager.get(), portfolio.get(), marketData.get()
        );
        auto authController = std::make_shared<AuthController>();
        
        // Create router and set up routes
        auto router = std::make_shared<Router>();
        
        // Health check endpoint
        router->get("/health", [](const HttpRequest& req) {
            HttpResponse res;
            res.headers["Content-Type"] = "application/json";
            res.body = R"({"status": "healthy", "service": "TradingSystem"})";
            return res;
        });
        
        // Trading endpoints
        router->post("/api/v1/orders", [orderManager](const HttpRequest& req) {
            HttpResponse res;
            res.headers["Content-Type"] = "application/json";
            
            try {
                // Parse order from request body (simplified)
                Order order;
                order.symbol = "AAPL"; // TODO: Parse from JSON
                order.type = OrderType::BUY;
                order.quantity = 100;
                order.price = 150.0;
                order.userId = "user123";
                
                std::string orderId = orderManager->createOrder(order);
                res.body = R"({"orderId": ")" + orderId + R"(", "status": "created"})";
            } catch (const std::exception& e) {
                res.statusCode = 400;
                res.statusText = "Bad Request";
                res.body = R"({"error": ")" + std::string(e.what()) + R"("})";
            }
            
            return res;
        });
        
        router->get("/api/v1/orders", [orderManager](const HttpRequest& req) {
            HttpResponse res;
            res.headers["Content-Type"] = "application/json";
            
            auto orders = orderManager->getActiveOrders();
            res.body = R"({"orders": [)";
            
            for (size_t i = 0; i < orders.size(); ++i) {
                if (i > 0) res.body += ", ";
                res.body += R"({"id": ")" + orders[i].id + R"(", "symbol": ")" + 
                           orders[i].symbol + R"("})";
            }
            
            res.body += "]}";
            return res;
        });
        
        router->get("/api/v1/market-data/:symbol", [marketData](const HttpRequest& req) {
            HttpResponse res;
            res.headers["Content-Type"] = "application/json";
            
            try {
                // Extract symbol from path (simplified)
                std::string symbol = "AAPL"; // TODO: Extract from path params
                double price = marketData->getCurrentPrice(symbol);
                
                res.body = R"({"symbol": ")" + symbol + R"(", "price": )" + 
                          std::to_string(price) + "}";
            } catch (const std::exception& e) {
                res.statusCode = 404;
                res.statusText = "Not Found";
                res.body = R"({"error": ")" + std::string(e.what()) + R"("})";
            }
            
            return res;
        });
        
        // Authentication endpoints
        router->post("/api/v1/auth/login", [authController](const HttpRequest& req) {
            HttpResponse res;
            res.headers["Content-Type"] = "application/json";
            
            // TODO: Parse credentials from request body
            bool success = authController->login("testuser", "testpass");
            
            if (success) {
                res.body = R"({"token": "dummy-jwt-token", "expiresIn": 3600})";
            } else {
                res.statusCode = 401;
                res.statusText = "Unauthorized";
                res.body = R"({"error": "Invalid credentials"})";
            }
            
            return res;
        });
        
        // Get port from config or use default
        int port = Constants::DEFAULT_PORT;
        std::string portStr = config.getValue("server.port");
        if (!portStr.empty()) {
            port = std::stoi(portStr);
        }
        
        // Create and start server
        g_server = std::make_unique<HttpServer>(port);
        g_server->setRouter(router);
        
        std::cout << "Trading System Server starting on port " << port << "..." << std::endl;
        std::cout << "Press Ctrl+C to stop the server" << std::endl;
        
        g_server->start();
        
        // Keep main thread alive
        while (g_server->isRunning()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        // Cleanup
        marketData->stopSimulation();
        dbManager->disconnect();
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}