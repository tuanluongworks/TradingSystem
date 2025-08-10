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

    // Order operations (implement IOrderRepository)
    bool save(const Order& order) override; // IOrderRepository
    std::optional<Order> findOrderById(const std::string& orderId) override; // renamed to avoid clash
    std::vector<Order> findOrdersByUserId(const std::string& userId) override; // renamed to avoid clash
    bool updateStatus(const std::string& orderId, OrderStatus status) override;

    // User operations (implement IUserRepository)
    bool save(const User& user) override; // IUserRepository
    std::optional<User> findUserById(const std::string& userId) override; // renamed
    std::optional<User> findByUsername(const std::string& username) override;

    // Asset operations (implement IAssetRepository)
    bool save(const std::string& userId, const Asset& asset) override; // IAssetRepository
    std::vector<Asset> findAssetsByUserId(const std::string& userId) override; // renamed
    bool update(const std::string& userId, const Asset& asset) override;

    // Market data operations (implement IMarketDataRepository)
    bool save(const MarketDataPoint& data) override; // IMarketDataRepository
    MarketDataPoint latest(const std::string& symbol) override;

private:
    bool initializeTables();
    std::string generateId();
};