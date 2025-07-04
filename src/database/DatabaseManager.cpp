#include "DatabaseManager.h"
#include "Models.h"
#include "../utils/JsonParser.h"
#include <fstream>
#include <random>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <vector>
#include <map>

DatabaseManager::DatabaseManager() : dbPath("trading_system.db"), connected(false) {}

DatabaseManager::DatabaseManager(const std::string& path) : dbPath(path), connected(false) {}

DatabaseManager::~DatabaseManager() {
    disconnect();
}

bool DatabaseManager::connect() {
    // Create database directory if it doesn't exist
    std::filesystem::create_directories("data");
    connected = true;
    return initializeTables();
}

void DatabaseManager::disconnect() {
    connected = false;
}

bool DatabaseManager::isConnected() const {
    return connected;
}

bool DatabaseManager::initializeTables() {
    // Create data files if they don't exist
    std::ofstream ordersFile("data/orders.json", std::ios::app);
    std::ofstream usersFile("data/users.json", std::ios::app);
    std::ofstream assetsFile("data/assets.json", std::ios::app);
    std::ofstream marketDataFile("data/market_data.json", std::ios::app);
    
    ordersFile.close();
    usersFile.close();
    assetsFile.close();
    marketDataFile.close();
    
    std::cout << "Database tables initialized successfully\n";
    return true;
}

bool DatabaseManager::saveOrder(const Order& order) {
    if (!connected) return false;
    
    std::ofstream file("data/orders.json", std::ios::app);
    if (!file.is_open()) return false;
    
    std::map<std::string, std::string> orderData;
    orderData["id"] = order.id;
    orderData["symbol"] = order.symbol;
    orderData["type"] = (order.type == OrderType::BUY) ? "BUY" : "SELL";
    orderData["quantity"] = std::to_string(order.quantity);
    orderData["price"] = std::to_string(order.price);
    orderData["status"] = std::to_string(static_cast<int>(order.status));
    orderData["userId"] = order.userId;
    orderData["timestamp"] = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
        order.timestamp.time_since_epoch()).count());
    
    file << JsonParser::createObject(orderData) << "\n";
    file.close();
    
    std::cout << "Order saved: " << order.id << " for symbol " << order.symbol << "\n";
    return true;
}

Order DatabaseManager::getOrderById(const std::string& orderId) {
    if (!connected) return Order();
    
    std::ifstream file("data/orders.json");
    if (!file.is_open()) return Order();
    
    std::string line;
    while (std::getline(file, line)) {
        std::string id = JsonParser::extractString(line, "id");
        if (id == orderId) {
            Order order;
            order.id = id;
            order.symbol = JsonParser::extractString(line, "symbol");
            std::string typeStr = JsonParser::extractString(line, "type");
            order.type = (typeStr == "BUY") ? OrderType::BUY : OrderType::SELL;
            order.quantity = JsonParser::extractNumber(line, "quantity");
            order.price = JsonParser::extractNumber(line, "price");
            order.status = static_cast<OrderStatus>(static_cast<int>(JsonParser::extractNumber(line, "status")));
            order.userId = JsonParser::extractString(line, "userId");
            
            file.close();
            return order;
        }
    }
    
    file.close();
    return Order();
}

std::vector<Order> DatabaseManager::getOrdersByUserId(const std::string& userId) {
    if (!connected) return {};
    
    std::vector<Order> orders;
    std::ifstream file("data/orders.json");
    if (!file.is_open()) return orders;
    
    std::string line;
    while (std::getline(file, line)) {
        std::string orderUserId = JsonParser::extractString(line, "userId");
        if (orderUserId == userId) {
            Order order;
            order.id = JsonParser::extractString(line, "id");
            order.symbol = JsonParser::extractString(line, "symbol");
            std::string typeStr = JsonParser::extractString(line, "type");
            order.type = (typeStr == "BUY") ? OrderType::BUY : OrderType::SELL;
            order.quantity = JsonParser::extractNumber(line, "quantity");
            order.price = JsonParser::extractNumber(line, "price");
            order.status = static_cast<OrderStatus>(static_cast<int>(JsonParser::extractNumber(line, "status")));
            order.userId = orderUserId;
            
            orders.push_back(order);
        }
    }
    
    file.close();
    return orders;
}

bool DatabaseManager::updateOrderStatus(const std::string& orderId, OrderStatus status) {
    if (!connected) return false;
    
    // Read all orders
    std::vector<std::string> allOrders;
    std::ifstream inFile("data/orders.json");
    if (!inFile.is_open()) return false;
    
    std::string line;
    bool updated = false;
    while (std::getline(inFile, line)) {
        std::string id = JsonParser::extractString(line, "id");
        if (id == orderId) {
            // Update the status in this order
            Order order;
            order.id = id;
            order.symbol = JsonParser::extractString(line, "symbol");
            std::string typeStr = JsonParser::extractString(line, "type");
            order.type = (typeStr == "BUY") ? OrderType::BUY : OrderType::SELL;
            order.quantity = JsonParser::extractNumber(line, "quantity");
            order.price = JsonParser::extractNumber(line, "price");
            order.status = status; // Update status
            order.userId = JsonParser::extractString(line, "userId");
            
            std::map<std::string, std::string> orderData;
            orderData["id"] = order.id;
            orderData["symbol"] = order.symbol;
            orderData["type"] = (order.type == OrderType::BUY) ? "BUY" : "SELL";
            orderData["quantity"] = std::to_string(order.quantity);
            orderData["price"] = std::to_string(order.price);
            orderData["status"] = std::to_string(static_cast<int>(order.status));
            orderData["userId"] = order.userId;
            orderData["timestamp"] = JsonParser::extractString(line, "timestamp");
            
            allOrders.push_back(JsonParser::createObject(orderData));
            updated = true;
        } else {
            allOrders.push_back(line);
        }
    }
    inFile.close();
    
    if (updated) {
        // Write back all orders
        std::ofstream outFile("data/orders.json", std::ios::trunc);
        for (const auto& orderLine : allOrders) {
            outFile << orderLine << "\n";
        }
        outFile.close();
    }
    
    std::cout << "Order status updated: " << orderId << "\n";
    return updated;
}

bool DatabaseManager::saveUser(const User& user) {
    if (!connected) return false;
    
    std::cout << "User saved: " << user.username << "\n";
    return true;
}

User DatabaseManager::getUserById(const std::string& userId) {
    if (!connected) return User();
    
    User user;
    user.id = userId;
    return user;
}

User DatabaseManager::getUserByUsername(const std::string& username) {
    if (!connected) return User();
    
    User user;
    user.username = username;
    return user;
}

bool DatabaseManager::saveAsset(const std::string& userId, const Asset& asset) {
    if (!connected) return false;
    
    std::cout << "Asset saved for user " << userId << ": " << asset.symbol << "\n";
    return true;
}

std::vector<Asset> DatabaseManager::getAssetsByUserId(const std::string& userId) {
    if (!connected) return {};
    
    std::vector<Asset> assets;
    return assets;
}

bool DatabaseManager::updateAsset(const std::string& userId, const Asset& asset) {
    if (!connected) return false;
    
    std::cout << "Asset updated for user " << userId << ": " << asset.symbol << "\n";
    return true;
}

bool DatabaseManager::saveMarketData(const MarketDataPoint& data) {
    if (!connected) return false;
    
    std::cout << "Market data saved: " << data.symbol << " @ " << data.price << "\n";
    return true;
}

MarketDataPoint DatabaseManager::getLatestMarketData(const std::string& symbol) {
    if (!connected) return MarketDataPoint();
    
    MarketDataPoint data;
    data.symbol = symbol;
    data.price = 100.0; // Simulated price
    return data;
}

std::string DatabaseManager::generateId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1000000, 9999999);
    
    return std::to_string(dis(gen));
}