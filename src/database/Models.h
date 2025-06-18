#pragma once
#include "Types.h"
#include <string>
#include <vector>

class DatabaseModels {
public:
    // Order table operations
    static std::string createOrdersTable();
    static std::string insertOrder();
    static std::string selectOrderById();
    static std::string selectOrdersByUserId();
    static std::string updateOrderStatus();
    
    // User table operations
    static std::string createUsersTable();
    static std::string insertUser();
    static std::string selectUserById();
    static std::string selectUserByUsername();
    
    // Portfolio table operations
    static std::string createPortfolioTable();
    static std::string insertAsset();
    static std::string selectAssetsByUserId();
    static std::string updateAssetQuantity();
    
    // Market data table operations
    static std::string createMarketDataTable();
    static std::string insertMarketData();
    static std::string selectLatestMarketData();
};