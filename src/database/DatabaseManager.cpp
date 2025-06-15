#include "DatabaseManager.h"
#include <iostream>
#include <stdexcept>

DatabaseManager::DatabaseManager(const std::string& connectionString) 
    : connectionString(connectionString) {
    // Initialize database connection
    connect();
}

DatabaseManager::~DatabaseManager() {
    // Clean up resources
    disconnect();
}

void DatabaseManager::connect() {
    // Logic to establish a database connection
    std::cout << "Connecting to database: " << connectionString << std::endl;
    // Simulate connection success
    isConnected = true;
}

void DatabaseManager::disconnect() {
    // Logic to close the database connection
    if (isConnected) {
        std::cout << "Disconnecting from database." << std::endl;
        isConnected = false;
    }
}

bool DatabaseManager::executeQuery(const std::string& query) {
    if (!isConnected) {
        throw std::runtime_error("Database not connected.");
    }
    // Logic to execute a query
    std::cout << "Executing query: " << query << std::endl;
    // Simulate query execution success
    return true;
}