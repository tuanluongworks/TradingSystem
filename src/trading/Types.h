#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <memory>
#include <cstdint>
#include <compare>

struct OrderId { std::string value; explicit OrderId(std::string v = {}) : value(std::move(v)) {} auto operator<=>(const OrderId&) const = default; };
struct UserId { std::string value; explicit UserId(std::string v = {}) : value(std::move(v)) {} auto operator<=>(const UserId&) const = default; };
struct Instrument { std::string value; explicit Instrument(std::string v = {}) : value(std::move(v)) {} auto operator<=>(const Instrument&) const = default; };
struct Quantity { double value{0.0}; explicit Quantity(double v = 0.0) : value(v) {} };
struct Price { double value{0.0}; explicit Price(double v = 0.0) : value(v) {} };

enum class OrderType { BUY, SELL };
enum class OrderStatus { PENDING, FILLED, CANCELLED, PARTIALLY_FILLED };
enum class LogLevel { LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_DEBUG };

struct Order {
    std::string id; // to be migrated to OrderId
    std::string symbol; // to Instrument
    OrderType type;
    double quantity; // to Quantity
    double price; // to Price
    OrderStatus status;
    std::chrono::system_clock::time_point timestamp;
    std::string userId; // to UserId
    Order() = default;
    Order(const std::string& id, const std::string& symbol, OrderType type, double quantity, double price, const std::string& userId)
        : id(id), symbol(symbol), type(type), quantity(quantity), price(price), status(OrderStatus::PENDING), userId(userId) { timestamp = std::chrono::system_clock::now(); }
};

struct Asset { std::string symbol; double quantity; double currentPrice; double averageCost; Asset() = default; Asset(const std::string& symbol, double quantity, double price): symbol(symbol), quantity(quantity), currentPrice(price), averageCost(price) {} };

struct MarketDataPoint { std::string symbol; double price; double volume; std::chrono::system_clock::time_point timestamp; MarketDataPoint() = default; MarketDataPoint(const std::string& symbol, double price, double volume): symbol(symbol), price(price), volume(volume) { timestamp = std::chrono::system_clock::now(); } };

struct User { std::string id; std::string username; std::string email; std::string passwordHash; bool isActive; std::chrono::system_clock::time_point createdAt; User() = default; User(const std::string& id, const std::string& username, const std::string& email): id(id), username(username), email(email), isActive(true) { createdAt = std::chrono::system_clock::now(); } };