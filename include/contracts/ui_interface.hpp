/**
 * UI Interface Contract
 * C++ Interface definitions for UI-Backend communication
 * Copy from specs/001-c-trading-system/contracts/ui_interface.hpp
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>

namespace trading::ui {

// Forward declarations
struct MarketDataRow;
struct OrderRow;
struct PositionRow;
struct TradeRow;
struct OrderFormData;

/**
 * UI Manager Interface
 * Manages the main application loop and UI state
 */
class IUIManager {
public:
    virtual ~IUIManager() = default;

    // Application Lifecycle
    virtual bool initialize() = 0;
    virtual void run() = 0;  // Main event loop
    virtual void shutdown() = 0;

    // Window Management
    virtual void show_market_data_window(bool show = true) = 0;
    virtual void show_order_entry_window(bool show = true) = 0;
    virtual void show_positions_window(bool show = true) = 0;
    virtual void show_trades_window(bool show = true) = 0;
    virtual void show_status_window(bool show = true) = 0;

    // Data Updates (called by backend)
    virtual void update_market_data(const std::vector<MarketDataRow>& data) = 0;
    virtual void update_orders(const std::vector<OrderRow>& orders) = 0;
    virtual void update_positions(const std::vector<PositionRow>& positions) = 0;
    virtual void update_trades(const std::vector<TradeRow>& trades) = 0;
    virtual void update_connection_status(bool connected, const std::string& status) = 0;

    // Event Callbacks (UI → Backend)
    virtual void set_order_submit_callback(std::function<void(const OrderFormData&)> callback) = 0;
    virtual void set_order_cancel_callback(std::function<void(const std::string&)> callback) = 0;
    virtual void set_symbol_subscribe_callback(std::function<void(const std::string&)> callback) = 0;
    virtual void set_symbol_unsubscribe_callback(std::function<void(const std::string&)> callback) = 0;
};

/**
 * Market Data Panel Interface
 * Real-time price display and subscription management
 */
class IMarketDataPanel {
public:
    virtual ~IMarketDataPanel() = default;

    // Display Operations
    virtual void render() = 0;
    virtual void update_data(const std::vector<MarketDataRow>& data) = 0;
    virtual void clear_data() = 0;

    // User Interactions
    virtual void set_symbol_click_callback(std::function<void(const std::string&)> callback) = 0;
    virtual void set_subscribe_callback(std::function<void(const std::string&)> callback) = 0;

    // Configuration
    virtual void set_auto_sort(bool enabled) = 0;
    virtual void set_precision(int decimal_places) = 0;
};

/**
 * Order Entry Panel Interface
 * Order creation and submission form
 */
class IOrderEntryPanel {
public:
    virtual ~IOrderEntryPanel() = default;

    // Display Operations
    virtual void render() = 0;
    virtual void reset_form() = 0;
    virtual void set_instrument(const std::string& symbol) = 0;

    // Form State
    virtual OrderFormData get_form_data() const = 0;
    virtual bool is_form_valid() const = 0;
    virtual std::string get_validation_error() const = 0;

    // Callbacks
    virtual void set_submit_callback(std::function<void(const OrderFormData&)> callback) = 0;
    virtual void set_clear_callback(std::function<void()> callback) = 0;
};

/**
 * Positions Panel Interface
 * Portfolio positions display and management
 */
class IPositionsPanel {
public:
    virtual ~IPositionsPanel() = default;

    // Display Operations
    virtual void render() = 0;
    virtual void update_data(const std::vector<PositionRow>& positions) = 0;
    virtual void clear_data() = 0;

    // User Interactions
    virtual void set_position_click_callback(std::function<void(const std::string&)> callback) = 0;
    virtual void set_close_position_callback(std::function<void(const std::string&)> callback) = 0;

    // Display Options
    virtual void set_show_pnl(bool show) = 0;
    virtual void set_show_unrealized(bool show) = 0;
};

// Data Transfer Objects for UI

struct MarketDataRow {
    std::string symbol;
    double bid_price;
    double ask_price;
    double last_price;
    double spread;
    double change_percent;
    std::chrono::system_clock::time_point last_update;
    bool is_stale;  // No updates for > threshold

    // Display helpers
    std::string get_formatted_price(double price, int precision = 2) const;
    std::string get_formatted_spread() const;
    std::string get_formatted_change() const;
};

struct OrderRow {
    std::string order_id;
    std::string symbol;
    std::string side;           // "BUY" or "SELL"
    std::string type;           // "MARKET" or "LIMIT"
    std::string status;         // Order status as string
    double quantity;
    double price;
    double filled_quantity;
    double remaining_quantity;
    std::chrono::system_clock::time_point created_time;
    std::chrono::system_clock::time_point last_modified;

    // Display helpers
    std::string get_formatted_time() const;
    std::string get_progress_text() const;  // "50/100 (50%)"
    bool is_cancelable() const;
};

struct PositionRow {
    std::string symbol;
    double quantity;
    double average_price;
    double current_price;
    double market_value;
    double unrealized_pnl;
    double realized_pnl;
    double total_pnl;
    double change_percent;

    // Display helpers
    std::string get_quantity_text() const;  // Include "LONG"/"SHORT"
    std::string get_formatted_pnl() const;
    std::string get_pnl_color() const;      // Green/Red based on P&L
};

struct TradeRow {
    std::string trade_id;
    std::string order_id;
    std::string symbol;
    std::string side;
    double quantity;
    double price;
    double notional_value;
    std::chrono::system_clock::time_point execution_time;

    // Display helpers
    std::string get_formatted_time() const;
    std::string get_formatted_value() const;
};

struct OrderFormData {
    std::string symbol;
    std::string side;           // "BUY" or "SELL"
    std::string type;           // "MARKET" or "LIMIT"
    double quantity;
    double price;               // 0 for market orders
    bool is_valid;
    std::string validation_error;

    // Validation
    bool validate();
    void clear();
};

} // namespace trading::ui