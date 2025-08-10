#include <iostream>
#include <memory>
#include <signal.h>
#include <cstdlib>
#include "server/HttpServer.h"
#include "server/Router.h"
#include "api/TradingController.h"
#include "api/AuthController.h"
#include "trading/OrderManager.h"
#include "trading/Portfolio.h"
#include "trading/MarketData.h"
#include "trading/MatchingEngine.h"
#include "trading/OrderEvents.h"
#include "infrastructure/LockFreeQueue.h"
#include "database/DatabaseManager.h"
#include "utils/Config.h"
#include "utils/Logger.h"
#include "utils/JsonParser.h"
#include "common/Constants.h"
#include "utils/ErrorResponse.h"
#include "common/Errors.h"
#include "middleware/RateLimiter.h"

// Global server pointer for signal handling
std::unique_ptr<HttpServer> g_server;
SPSCQueue<TradingEvent>* g_orderEventQueue = nullptr; // global pointer used by managers

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
        auto orderManager = std::make_shared<OrderManager>(dbManager); // IOrderService
        auto marketData = std::make_shared<MarketData>(dbManager);     // IMarketDataService
        auto portfolio = std::make_shared<Portfolio>("default_user", dbManager); // IPortfolioService
        
        // Start market data simulation
        marketData->startSimulation();
        
        // Initialize controllers
        auto tradingController = std::make_shared<TradingController>(
            orderManager.get(), portfolio.get(), marketData.get()
        );
        auto authController = std::make_shared<AuthController>();
        
        // Router creation
        auto router = std::make_shared<Router>();
        
        // Add rate limiting (token bucket) before routes
        router->use(RateLimiter::createTokenBucket(TokenBucketConfig{ .capacity = 50, .refillTokensPerSecond = 10.0 }));
        
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
                // Parse order from request body
                Order order;
                order.symbol = JsonParser::extractString(req.body, "symbol");
                std::string typeStr = JsonParser::extractString(req.body, "type");
                order.type = (typeStr == "BUY" || typeStr == "buy") ? OrderType::BUY : OrderType::SELL;
                order.quantity = JsonParser::extractNumber(req.body, "quantity");
                order.price = JsonParser::extractNumber(req.body, "price");
                order.userId = "user123"; // TODO: Extract from auth token
                
                // Validate required fields
                if (order.symbol.empty() || order.quantity <= 0 || order.price <= 0) {
                    res.statusCode = 400;
                    res.statusText = "Bad Request";
                    res.body = R"({"error": "Invalid order parameters"})";
                    return res;
                }
                
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
                           orders[i].symbol + R"(", "type": ")" + 
                           (orders[i].type == OrderType::BUY ? "BUY" : "SELL") + 
                           R"(", "quantity": )" + std::to_string(orders[i].quantity) +
                           R"(, "price": )" + std::to_string(orders[i].price) + "}";
            }
            
            res.body += "]}";
            return res;
        });
        
        router->del("/api/v1/orders/:orderId", [orderManager](const HttpRequest& req) {
            HttpResponse res;
            res.headers["Content-Type"] = "application/json";
            
            try {
                std::string orderId = req.pathParams.at("orderId");
                bool success = orderManager->cancelOrder(orderId);
                
                if (success) {
                    res.body = R"({"success": true, "message": "Order cancelled successfully"})";
                } else {
                    res.statusCode = 404;
                    res.statusText = "Not Found";
                    res.body = R"({"error": "Order not found or already processed"})";
                }
            } catch (const std::exception& e) {
                res.statusCode = 400;
                res.statusText = "Bad Request";
                res.body = R"({"error": ")" + std::string(e.what()) + R"("})";
            }
            
            return res;
        });
        
        router->get("/api/v1/market-data/:symbol", [marketData](const HttpRequest& req) {
            HttpResponse res;
            res.headers["Content-Type"] = "application/json";
            
            try {
                // Extract symbol from path parameters
                std::string symbol = req.pathParams.at("symbol");
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
        
        // Get all market data
        router->get("/api/v1/market-data", [marketData](const HttpRequest& req) {
            HttpResponse res;
            res.headers["Content-Type"] = "application/json";
            
            try {
                auto symbols = marketData->getAvailableSymbols();
                res.body = R"({"success": true, "marketData": [)";
                
                for (size_t i = 0; i < symbols.size(); ++i) {
                    if (i > 0) res.body += ", ";
                    auto data = marketData->getLatestData(symbols[i]);
                    res.body += R"({"symbol": ")" + symbols[i] + 
                               R"(", "price": )" + std::to_string(data.price) +
                               R"(, "volume": )" + std::to_string(data.volume) + "}";
                }
                
                res.body += "]}";
            } catch (const std::exception& e) {
                res.statusCode = 500;
                res.statusText = "Internal Server Error";
                res.body = R"({"error": ")" + std::string(e.what()) + R"("})";
            }
            
            return res;
        });
        
        // Portfolio endpoint
        router->get("/api/v1/portfolio", [portfolio](const HttpRequest& req) {
            HttpResponse res;
            res.headers["Content-Type"] = "application/json";
            
            try {
                const auto& assets = portfolio->getAssets();
                double totalValue = portfolio->getTotalValue();
                
                res.body = R"({"success": true, "totalValue": )" + std::to_string(totalValue) + 
                          R"(, "assets": [)";
                
                for (size_t i = 0; i < assets.size(); ++i) {
                    if (i > 0) res.body += ", ";
                    const auto& asset = assets[i];
                    res.body += R"({"symbol": ")" + asset.symbol + 
                               R"(", "quantity": )" + std::to_string(asset.quantity) +
                               R"(, "currentPrice": )" + std::to_string(asset.currentPrice) +
                               R"(, "averageCost": )" + std::to_string(asset.averageCost) + "}";
                }
                
                res.body += "]}";
            } catch (const std::exception& e) {
                res.statusCode = 500;
                res.statusText = "Internal Server Error";
                res.body = R"({"error": ")" + std::string(e.what()) + R"("})";
            }
            
            return res;
        });
        
        // Authentication endpoints
        router->post("/api/v1/auth/login", [authController](const HttpRequest& req) {
            HttpResponse res; res.headers["Content-Type"] = "application/json";
            try {
                std::string username = JsonParser::extractString(req.body, "username");
                std::string password = JsonParser::extractString(req.body, "password");
                if (username.empty() || password.empty()) { auto err = Error::validation("Username and password are required"); res.statusCode = 400; res.statusText = "Bad Request"; res.body = buildErrorJson(err); return res; }
                bool success = authController->login(username, password);
                if (success) { std::string userId = "user123"; std::string token = authController->generateAuthToken(userId, username); res.body = std::string("{\"token\":\"") + token + "\",\"expiresIn\":3600}"; }
                else { auto err = Error::auth("Invalid credentials"); res.statusCode = 401; res.statusText = "Unauthorized"; res.body = buildErrorJson(err); }
            } catch(const std::exception& e) { auto err = Error::internal("Unhandled exception", e.what()); res.statusCode = 500; res.statusText = "Internal Server Error"; res.body = buildErrorJson(err); }
            return res; });
        
        // Get port from config or use default
        int port = Constants::DEFAULT_PORT;
        std::string portStr = config.getValue("server.port");
        if (!portStr.empty()) {
            port = std::stoi(portStr);
        }
        
        // Create and start server
        g_server = std::make_unique<HttpServer>(port);
        g_server->setRouter(router);
        
        static SPSCQueue<TradingEvent> eventQueue(1024); // power-of-two capacity
        g_orderEventQueue = &eventQueue;
        MatchingEngine engine(eventQueue);
        engine.start();
        
        std::cout << "Trading System Server starting on port " << port << "..." << std::endl;
        std::cout << "Press Ctrl+C to stop the server" << std::endl;
        
        g_server->start();
        
        // Keep main thread alive
        while (g_server->isRunning()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        // Cleanup
        engine.stop();
        marketData->stopSimulation();
        dbManager->disconnect();
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}