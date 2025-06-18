#include "Models.h"

std::string DatabaseModels::createOrdersTable() {
    return R"(
        CREATE TABLE IF NOT EXISTS orders (
            id TEXT PRIMARY KEY,
            symbol TEXT NOT NULL,
            type INTEGER NOT NULL,
            quantity REAL NOT NULL,
            price REAL NOT NULL,
            status INTEGER NOT NULL,
            timestamp INTEGER NOT NULL,
            user_id TEXT NOT NULL
        )
    )";
}

std::string DatabaseModels::insertOrder() {
    return "INSERT INTO orders (id, symbol, type, quantity, price, status, timestamp, user_id) VALUES (?, ?, ?, ?, ?, ?, ?, ?)";
}

std::string DatabaseModels::selectOrderById() {
    return "SELECT * FROM orders WHERE id = ?";
}

std::string DatabaseModels::selectOrdersByUserId() {
    return "SELECT * FROM orders WHERE user_id = ?";
}

std::string DatabaseModels::updateOrderStatus() {
    return "UPDATE orders SET status = ? WHERE id = ?";
}

std::string DatabaseModels::createUsersTable() {
    return R"(
        CREATE TABLE IF NOT EXISTS users (
            id TEXT PRIMARY KEY,
            username TEXT UNIQUE NOT NULL,
            email TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            is_active INTEGER NOT NULL DEFAULT 1,
            created_at INTEGER NOT NULL
        )
    )";
}

std::string DatabaseModels::insertUser() {
    return "INSERT INTO users (id, username, email, password_hash, is_active, created_at) VALUES (?, ?, ?, ?, ?, ?)";
}

std::string DatabaseModels::selectUserById() {
    return "SELECT * FROM users WHERE id = ?";
}

std::string DatabaseModels::selectUserByUsername() {
    return "SELECT * FROM users WHERE username = ?";
}

std::string DatabaseModels::createPortfolioTable() {
    return R"(
        CREATE TABLE IF NOT EXISTS portfolio (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id TEXT NOT NULL,
            symbol TEXT NOT NULL,
            quantity REAL NOT NULL,
            average_cost REAL NOT NULL,
            UNIQUE(user_id, symbol)
        )
    )";
}

std::string DatabaseModels::insertAsset() {
    return "INSERT OR REPLACE INTO portfolio (user_id, symbol, quantity, average_cost) VALUES (?, ?, ?, ?)";
}

std::string DatabaseModels::selectAssetsByUserId() {
    return "SELECT * FROM portfolio WHERE user_id = ?";
}

std::string DatabaseModels::updateAssetQuantity() {
    return "UPDATE portfolio SET quantity = ?, average_cost = ? WHERE user_id = ? AND symbol = ?";
}

std::string DatabaseModels::createMarketDataTable() {
    return R"(
        CREATE TABLE IF NOT EXISTS market_data (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            symbol TEXT NOT NULL,
            price REAL NOT NULL,
            volume REAL NOT NULL,
            timestamp INTEGER NOT NULL
        )
    )";
}

std::string DatabaseModels::insertMarketData() {
    return "INSERT INTO market_data (symbol, price, volume, timestamp) VALUES (?, ?, ?, ?)";
}

std::string DatabaseModels::selectLatestMarketData() {
    return "SELECT * FROM market_data WHERE symbol = ? ORDER BY timestamp DESC LIMIT 1";
}