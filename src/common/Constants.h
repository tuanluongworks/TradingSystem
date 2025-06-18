#pragma once

namespace Constants {
    // Server Configuration
    constexpr int DEFAULT_PORT = 8080;
    constexpr const char* DEFAULT_HOST = "localhost";
    constexpr int MAX_CONNECTIONS = 100;
    
    // Trading Configuration
    constexpr double MIN_ORDER_QUANTITY = 0.01;
    constexpr double MAX_ORDER_QUANTITY = 1000000.0;
    constexpr double MIN_ORDER_PRICE = 0.01;
    
    // Database Configuration
    constexpr const char* DEFAULT_DB_PATH = "trading_system.db";
    
    // API Endpoints
    constexpr const char* API_PREFIX = "/api/v1";
    constexpr const char* ORDERS_ENDPOINT = "/orders";
    constexpr const char* PORTFOLIO_ENDPOINT = "/portfolio";
    constexpr const char* MARKET_DATA_ENDPOINT = "/market-data";
    constexpr const char* AUTH_ENDPOINT = "/auth";
}