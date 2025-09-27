/**
 * Trading Engine API Contract
 * C++ Interface definitions for core trading operations
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>

namespace trading {

// Enums
enum class OrderSide {
    BUY,
    SELL
};

enum class OrderType {
    MARKET,
    LIMIT
};

enum class OrderStatus {
    NEW,              // Just created, not yet sent
    ACCEPTED,         // Accepted by execution engine
    PARTIALLY_FILLED, // Some quantity executed
    FILLED,           // Completely executed
    CANCELED,         // Canceled by user
    REJECTED          // Rejected by risk management or engine
};

// Forward declarations
class Order;
class Trade;
class Position;
class Instrument;
class MarketTick;
struct OrderRequest;
struct ExecutionReport;

/**
 * Core Trading Engine Interface
 * Manages order lifecycle, execution, and position tracking
 */
class ITradingEngine {
public:
    virtual ~ITradingEngine() = default;

    // Order Management
    virtual std::string submit_order(const OrderRequest& request) = 0;
    virtual bool cancel_order(const std::string& order_id) = 0;
    virtual bool modify_order(const std::string& order_id, double new_quantity, double new_price) = 0;

    // Query Operations
    virtual std::shared_ptr<Order> get_order(const std::string& order_id) const = 0;
    virtual std::vector<std::shared_ptr<Order>> get_working_orders() const = 0;
    virtual std::vector<std::shared_ptr<Order>> get_orders_by_symbol(const std::string& symbol) const = 0;

    // Position Management
    virtual std::shared_ptr<Position> get_position(const std::string& symbol) const = 0;
    virtual std::vector<std::shared_ptr<Position>> get_all_positions() const = 0;

    // Trade History
    virtual std::vector<std::shared_ptr<Trade>> get_trades_by_order(const std::string& order_id) const = 0;
    virtual std::vector<std::shared_ptr<Trade>> get_trades_by_symbol(const std::string& symbol) const = 0;
    virtual std::vector<std::shared_ptr<Trade>> get_daily_trades() const = 0;

    // Event Callbacks
    virtual void set_order_update_callback(std::function<void(const ExecutionReport&)> callback) = 0;
    virtual void set_trade_callback(std::function<void(const Trade&)> callback) = 0;
    virtual void set_position_update_callback(std::function<void(const Position&)> callback) = 0;
};

/**
 * Market Data Interface
 * Manages real-time market data connections and distribution
 */
class IMarketDataProvider {
public:
    virtual ~IMarketDataProvider() = default;

    // Connection Management
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool is_connected() const = 0;

    // Subscription Management
    virtual bool subscribe(const std::string& symbol) = 0;
    virtual bool unsubscribe(const std::string& symbol) = 0;
    virtual std::vector<std::string> get_subscribed_symbols() const = 0;

    // Data Access
    virtual std::shared_ptr<MarketTick> get_latest_tick(const std::string& symbol) const = 0;
    virtual std::vector<std::shared_ptr<MarketTick>> get_recent_ticks(const std::string& symbol, int count) const = 0;

    // Event Callbacks
    virtual void set_tick_callback(std::function<void(const MarketTick&)> callback) = 0;
    virtual void set_connection_callback(std::function<void(bool connected)> callback) = 0;
};

/**
 * Risk Management Interface
 * Pre-trade risk validation and limit enforcement
 */
class IRiskManager {
public:
    virtual ~IRiskManager() = default;

    // Risk Validation
    virtual bool validate_order(const OrderRequest& request) const = 0;
    virtual std::string get_rejection_reason(const OrderRequest& request) const = 0;

    // Limit Management
    virtual bool set_position_limit(const std::string& symbol, double max_quantity) = 0;
    virtual bool set_order_size_limit(const std::string& symbol, double max_quantity) = 0;
    virtual bool set_daily_loss_limit(double max_loss) = 0;

    // Limit Queries
    virtual double get_position_limit(const std::string& symbol) const = 0;
    virtual double get_order_size_limit(const std::string& symbol) const = 0;
    virtual double get_daily_loss_limit() const = 0;

    // Risk Metrics
    virtual double get_current_exposure(const std::string& symbol) const = 0;
    virtual double get_daily_pnl() const = 0;
    virtual double get_total_position_value() const = 0;
};

/**
 * Persistence Interface
 * Data storage and retrieval operations
 */
class IPersistenceService {
public:
    virtual ~IPersistenceService() = default;

    // Trade Persistence
    virtual bool save_trade(const Trade& trade) = 0;
    virtual bool save_order(const Order& order) = 0;
    virtual bool update_position(const Position& position) = 0;

    // Data Retrieval
    virtual std::vector<std::shared_ptr<Trade>> load_trades_by_date(const std::chrono::system_clock::time_point& date) = 0;
    virtual std::vector<std::shared_ptr<Order>> load_orders_by_date(const std::chrono::system_clock::time_point& date) = 0;
    virtual std::vector<std::shared_ptr<Position>> load_all_positions() = 0;

    // Backup Operations
    virtual bool backup_to_file(const std::string& filepath) = 0;
    virtual bool restore_from_file(const std::string& filepath) = 0;

    // Health Check
    virtual bool is_available() const = 0;
    virtual std::string get_status() const = 0;
};

// Data Transfer Objects

struct OrderRequest {
    std::string instrument_symbol;
    OrderSide side;
    OrderType type;
    double quantity;
    double price;  // 0 for market orders
    std::chrono::system_clock::time_point timestamp;

    bool is_valid() const;
};

struct ExecutionReport {
    std::string order_id;
    OrderStatus old_status;
    OrderStatus new_status;
    double filled_quantity;
    double remaining_quantity;
    double execution_price;
    std::chrono::system_clock::time_point timestamp;
    std::string rejection_reason;  // If status == REJECTED
};

struct PositionSummary {
    std::string instrument_symbol;
    double quantity;
    double average_price;
    double current_price;
    double unrealized_pnl;
    double realized_pnl;
    double market_value;
};

struct TradingSummary {
    std::chrono::system_clock::time_point date;
    int total_orders;
    int filled_orders;
    double total_volume;
    double total_pnl;
    int active_positions;
    std::vector<PositionSummary> positions;
};

} // namespace trading