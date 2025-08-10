#pragma once
#include "../trading/Types.h"
#include <string>
#include <vector>
#include <memory>
#include "IOrderRepository.h"
#include "IUserRepository.h"
#include "IAssetRepository.h"
#include "IMarketDataRepository.h"

// Simple database interface without external dependencies
class DatabaseManager : public IOrderRepository, public IUserRepository, public IAssetRepository, public IMarketDataRepository {
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
    bool save(const Order& order) override; // IOrderRepository
    std::optional<Order> findById(const std::string& orderId) override;
    std::vector<Order> findByUserId(const std::string& userId) override;
    bool updateStatus(const std::string& orderId, OrderStatus status) override;

    // User operations
    bool save(const User& user) override; // IUserRepository
    std::optional<User> findById(const std::string& userId) override; // IUserRepository
    std::optional<User> findByUsername(const std::string& username) override;

    // Portfolio operations
    bool save(const std::string& userId, const Asset& asset) override; // IAssetRepository
    std::vector<Asset> findByUserId(const std::string& userId) override; // IAssetRepository conflict resolved by scope, will adjust names if needed
    bool update(const std::string& userId, const Asset& asset) override;

    // Market data operations
    bool save(const MarketDataPoint& data) override; // IMarketDataRepository
    MarketDataPoint latest(const std::string& symbol) override;

private:
    bool initializeTables();
    std::string generateId();
};