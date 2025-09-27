#pragma once

#include "../../contracts/trading_engine_api.hpp"
#include "../models/order.hpp"
#include "../models/trade.hpp"
#include "../models/position.hpp"
#include "../models/instrument.hpp"
#include "../risk/risk_manager.hpp"
#include "../messaging/message_queue.hpp"
#include "../../infrastructure/persistence/sqlite_service.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>
#include <chrono>

namespace trading {

// Forward declarations
class RiskManager;
class SQLiteService;
class MarketDataProvider;

/**
 * Trading Engine Implementation
 * Manages complete order lifecycle, execution, and position tracking
 */
class TradingEngine : public ITradingEngine {
public:
    explicit TradingEngine(
        std::shared_ptr<RiskManager> risk_manager,
        std::shared_ptr<SQLiteService> persistence_service = nullptr
    );
    virtual ~TradingEngine();

    // Initialization
    bool initialize();
    void shutdown();

    // ITradingEngine implementation
    std::string submit_order(const OrderRequest& request) override;
    bool cancel_order(const std::string& order_id) override;
    bool modify_order(const std::string& order_id, double new_quantity, double new_price) override;

    // Query operations
    std::shared_ptr<Order> get_order(const std::string& order_id) const override;
    std::vector<std::shared_ptr<Order>> get_working_orders() const override;
    std::vector<std::shared_ptr<Order>> get_orders_by_symbol(const std::string& symbol) const override;

    // Position management
    std::shared_ptr<Position> get_position(const std::string& symbol) const override;
    std::vector<std::shared_ptr<Position>> get_all_positions() const override;

    // Trade history
    std::vector<std::shared_ptr<Trade>> get_trades_by_order(const std::string& order_id) const override;
    std::vector<std::shared_ptr<Trade>> get_trades_by_symbol(const std::string& symbol) const override;
    std::vector<std::shared_ptr<Trade>> get_daily_trades() const override;

    // Event callbacks
    void set_order_update_callback(std::function<void(const ExecutionReport&)> callback) override;
    void set_trade_callback(std::function<void(const Trade&)> callback) override;
    void set_position_update_callback(std::function<void(const Position&)> callback) override;

    // Additional functionality
    void set_market_data_provider(std::shared_ptr<class IMarketDataProvider> provider);

    // Statistics
    size_t get_order_count() const;
    size_t get_trade_count() const;
    size_t get_position_count() const;

    // Engine status
    bool is_running() const;
    std::string get_engine_status() const;

    // Manual execution (for testing/simulation)
    bool execute_order(const std::string& order_id, double quantity, double price);

private:
    // Dependencies
    std::shared_ptr<RiskManager> risk_manager_;
    std::shared_ptr<SQLiteService> persistence_service_;
    std::shared_ptr<class IMarketDataProvider> market_data_provider_;

    // Engine state
    std::atomic<bool> is_running_;
    std::atomic<bool> should_stop_;
    mutable std::mutex engine_mutex_;

    // Order management
    std::unordered_map<std::string, std::shared_ptr<Order>> orders_;
    std::unordered_map<std::string, std::vector<std::string>> orders_by_symbol_;
    std::atomic<size_t> order_sequence_;

    // Position management
    std::unordered_map<std::string, std::shared_ptr<Position>> positions_;

    // Trade tracking
    std::vector<std::shared_ptr<Trade>> trades_;
    std::unordered_map<std::string, std::vector<std::shared_ptr<Trade>>> trades_by_order_;
    std::unordered_map<std::string, std::vector<std::shared_ptr<Trade>>> trades_by_symbol_;
    std::atomic<size_t> trade_sequence_;

    // Callbacks
    std::function<void(const ExecutionReport&)> order_update_callback_;
    std::function<void(const Trade&)> trade_callback_;
    std::function<void(const Position&)> position_update_callback_;

    // Message processing
    MessageQueue<std::function<void()>> order_processing_queue_;
    std::thread order_processing_thread_;

    // Helper methods
    std::string generate_order_id();
    std::string generate_trade_id();

    // Order lifecycle
    bool validate_order_request(const OrderRequest& request) const;
    std::shared_ptr<Order> create_order(const OrderRequest& request);
    bool accept_order(std::shared_ptr<Order> order);
    bool reject_order(std::shared_ptr<Order> order, const std::string& reason);

    // Order execution
    void execute_market_order(std::shared_ptr<Order> order);
    void execute_limit_order(std::shared_ptr<Order> order);
    bool can_execute_order(std::shared_ptr<Order> order, double market_price) const;

    // Trade processing
    std::shared_ptr<Trade> create_trade(
        std::shared_ptr<Order> order,
        double quantity,
        double price,
        TradeType type = TradeType::FULL_FILL
    );
    void process_trade(std::shared_ptr<Trade> trade);

    // Position management
    void update_position(std::shared_ptr<Trade> trade);
    std::shared_ptr<Position> get_or_create_position(const std::string& symbol);

    // Event notifications
    void notify_order_update(std::shared_ptr<Order> order, OrderStatus old_status);
    void notify_trade(std::shared_ptr<Trade> trade);
    void notify_position_update(std::shared_ptr<Position> position);

    // Persistence
    void persist_order(std::shared_ptr<Order> order);
    void persist_trade(std::shared_ptr<Trade> trade);
    void persist_position(std::shared_ptr<Position> position);

    // Order processing thread
    void process_orders();

    // Market data integration
    double get_market_price(const std::string& symbol, OrderType order_type) const;

    // Utility methods
    void add_order_to_symbol_index(const std::string& symbol, const std::string& order_id);
    void remove_order_from_symbol_index(const std::string& symbol, const std::string& order_id);

    // Logging
    void log_order_event(const std::string& event, std::shared_ptr<Order> order) const;
    void log_trade_event(const std::string& event, std::shared_ptr<Trade> trade) const;
    void log_engine_event(const std::string& event) const;
};

/**
 * Order Manager Helper Class
 * Manages order state transitions and validation
 */
class OrderManager {
public:
    static bool is_valid_status_transition(OrderStatus from, OrderStatus to);
    static std::string get_transition_error(OrderStatus from, OrderStatus to);
    static bool is_working_status(OrderStatus status);
    static bool is_terminal_status(OrderStatus status);
};

/**
 * Position Calculator
 * Handles position calculations and P&L
 */
class PositionCalculator {
public:
    static void update_position_with_trade(Position& position, const Trade& trade);
    static double calculate_unrealized_pnl(const Position& position, double current_price);
    static double calculate_realized_pnl(const Position& position, const Trade& closing_trade);
};

/**
 * Execution Engine Configuration
 */
struct ExecutionConfig {
    bool enable_simulation = true;          // Use simulation execution
    double latency_simulation_ms = 1.0;     // Simulated execution latency
    double slippage_bps = 1.0;              // Price slippage in basis points
    bool enable_partial_fills = true;       // Allow partial order execution
    size_t max_working_orders = 1000;       // Maximum working orders
    size_t max_daily_trades = 10000;        // Maximum daily trades
};

} // namespace trading