#pragma once

#include "contracts/trading_engine_api.hpp"
#include <sqlite_orm/sqlite_orm.h>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <optional>

namespace trading {

// Forward declarations for ORM mapping
class Order;
class Trade;
class Position;

// Database row structures for ORM
struct OrderRow {
    std::string order_id;
    std::string instrument_symbol;
    int side;                    // OrderSide as int
    int type;                    // OrderType as int
    double quantity;
    double price;
    int status;                  // OrderStatus as int
    double filled_quantity;
    double total_fill_value;
    std::int64_t created_time;   // Unix timestamp
    std::int64_t last_modified;  // Unix timestamp
    std::string rejection_reason;
};

struct TradeRow {
    std::string trade_id;
    std::string order_id;
    std::string instrument_symbol;
    int side;                    // OrderSide as int
    double quantity;
    double price;
    std::int64_t execution_time; // Unix timestamp
    int type;                    // TradeType as int
};

struct PositionRow {
    std::string instrument_symbol;
    double quantity;
    double average_price;
    double realized_pnl;
    double unrealized_pnl;
    std::int64_t last_updated;   // Unix timestamp
};

/**
 * SQLite Persistence Service
 * Implements ACID-compliant storage for trading data using sqlite_orm
 */
class SQLiteService : public IPersistenceService {
public:
    explicit SQLiteService(const std::string& database_path);
    virtual ~SQLiteService();

    // Database initialization
    bool initialize();
    void close();

    // IPersistenceService implementation
    bool save_trade(const Trade& trade) override;
    bool save_order(const Order& order) override;
    bool update_position(const Position& position) override;

    std::vector<std::shared_ptr<Trade>> load_trades_by_date(
        const std::chrono::system_clock::time_point& date) override;
    std::vector<std::shared_ptr<Order>> load_orders_by_date(
        const std::chrono::system_clock::time_point& date) override;
    std::vector<std::shared_ptr<Position>> load_all_positions() override;

    bool backup_to_file(const std::string& filepath) override;
    bool restore_from_file(const std::string& filepath) override;

    bool is_available() const override;
    std::string get_status() const override;

    // Additional query methods
    std::vector<std::shared_ptr<Trade>> load_trades_by_symbol(const std::string& symbol);
    std::vector<std::shared_ptr<Order>> load_orders_by_symbol(const std::string& symbol);
    std::shared_ptr<Position> load_position_by_symbol(const std::string& symbol);

    // Statistics
    size_t get_trade_count() const;
    size_t get_order_count() const;
    size_t get_position_count() const;

private:
    std::string database_path_;
    bool is_initialized_;
    mutable std::mutex database_mutex_;

    // sqlite_orm storage type
    using Storage = decltype(sqlite_orm::make_storage("",
        sqlite_orm::make_table("orders",
            sqlite_orm::make_column("order_id", &OrderRow::order_id, sqlite_orm::primary_key()),
            sqlite_orm::make_column("instrument_symbol", &OrderRow::instrument_symbol),
            sqlite_orm::make_column("side", &OrderRow::side),
            sqlite_orm::make_column("type", &OrderRow::type),
            sqlite_orm::make_column("quantity", &OrderRow::quantity),
            sqlite_orm::make_column("price", &OrderRow::price),
            sqlite_orm::make_column("status", &OrderRow::status),
            sqlite_orm::make_column("filled_quantity", &OrderRow::filled_quantity),
            sqlite_orm::make_column("total_fill_value", &OrderRow::total_fill_value),
            sqlite_orm::make_column("created_time", &OrderRow::created_time),
            sqlite_orm::make_column("last_modified", &OrderRow::last_modified),
            sqlite_orm::make_column("rejection_reason", &OrderRow::rejection_reason)
        ),
        sqlite_orm::make_table("trades",
            sqlite_orm::make_column("trade_id", &TradeRow::trade_id, sqlite_orm::primary_key()),
            sqlite_orm::make_column("order_id", &TradeRow::order_id),
            sqlite_orm::make_column("instrument_symbol", &TradeRow::instrument_symbol),
            sqlite_orm::make_column("side", &TradeRow::side),
            sqlite_orm::make_column("quantity", &TradeRow::quantity),
            sqlite_orm::make_column("price", &TradeRow::price),
            sqlite_orm::make_column("execution_time", &TradeRow::execution_time),
            sqlite_orm::make_column("type", &TradeRow::type)
        ),
        sqlite_orm::make_table("positions",
            sqlite_orm::make_column("instrument_symbol", &PositionRow::instrument_symbol, sqlite_orm::primary_key()),
            sqlite_orm::make_column("quantity", &PositionRow::quantity),
            sqlite_orm::make_column("average_price", &PositionRow::average_price),
            sqlite_orm::make_column("realized_pnl", &PositionRow::realized_pnl),
            sqlite_orm::make_column("unrealized_pnl", &PositionRow::unrealized_pnl),
            sqlite_orm::make_column("last_updated", &PositionRow::last_updated)
        )
    ));

    std::unique_ptr<Storage> storage_;

    // Helper methods
    OrderRow order_to_row(const Order& order) const;
    std::shared_ptr<Order> row_to_order(const OrderRow& row) const;

    TradeRow trade_to_row(const Trade& trade) const;
    std::shared_ptr<Trade> row_to_trade(const TradeRow& row) const;

    PositionRow position_to_row(const Position& position) const;
    std::shared_ptr<Position> row_to_position(const PositionRow& row) const;

    // Time conversion helpers
    std::int64_t timepoint_to_unix(const std::chrono::system_clock::time_point& tp) const;
    std::chrono::system_clock::time_point unix_to_timepoint(std::int64_t unix_time) const;

    // Date range helpers
    std::pair<std::int64_t, std::int64_t> get_date_range(
        const std::chrono::system_clock::time_point& date) const;

    // Error handling
    void log_error(const std::string& operation, const std::exception& e) const;
    bool handle_database_error(const std::string& operation) const;
};

} // namespace trading