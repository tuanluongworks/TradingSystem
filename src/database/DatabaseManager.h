#pragma once
#include "Types.h"
#include <string>
#include <vector>
#include <memory>

// Simple database interface without external dependencies
class DatabaseManager {
private:
    std::string dbPath;
    bool connected;

public:
    DatabaseManager();
    explicit DatabaseManager(const std::string& path);
    ~DatabaseManager();

    bool connect();
    void disconnect();
    bool isConnected() const;

    // Order operations
    bool saveOrder(const Order& order);
    Order getOrderById(const std::string& orderId);
    std::vector<Order> getOrdersByUserId(const std::string& userId);
    bool updateOrderStatus(const std::string& orderId, OrderStatus status);

    // User operations
    bool saveUser(const User& user);
    User getUserById(const std::string& userId);
    User getUserByUsername(const std::string& username);

    // Portfolio operations
    bool saveAsset(const std::string& userId, const Asset& asset);
    std::vector<Asset> getAssetsByUserId(const std::string& userId);
    bool updateAsset(const std::string& userId, const Asset& asset);

    // Market data operations
    bool saveMarketData(const MarketDataPoint& data);
    MarketDataPoint getLatestMarketData(const std::string& symbol);

private:
    bool initializeTables();
    std::string generateId();
};