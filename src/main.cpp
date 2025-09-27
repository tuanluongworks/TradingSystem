#include <iostream>
#include <memory>
#include <signal.h>
#include <thread>
#include <chrono>

// Core components
#include "core/engine/trading_engine.hpp"
#include "core/risk/risk_manager.hpp"
#include "infrastructure/persistence/sqlite_service.hpp"
#include "infrastructure/market_data/market_data_provider.hpp"

// UI components
#include "ui/managers/ui_manager.hpp"
#include "ui/rendering/opengl_context.hpp"
#include "ui/components/market_data_panel.hpp"
#include "ui/components/order_entry_panel.hpp"
#include "ui/components/positions_panel.hpp"
#include "ui/components/trades_panel.hpp"
#include "ui/components/status_panel.hpp"

// Utilities
#include "utils/config.hpp"
#include "utils/logging.hpp"
#include "utils/exceptions.hpp"

using namespace trading;

class TradingApplication {
private:
    // Core components
    std::shared_ptr<Config> config_;
    std::shared_ptr<PersistenceService> persistence_;
    std::shared_ptr<RiskManager> risk_manager_;
    std::shared_ptr<MarketDataProvider> market_data_provider_;
    std::shared_ptr<TradingEngine> trading_engine_;

    // UI components
    std::shared_ptr<ui::UIManager> ui_manager_;
    std::shared_ptr<ui::MarketDataPanel> market_data_panel_;
    std::shared_ptr<ui::OrderEntryPanel> order_entry_panel_;
    std::shared_ptr<ui::PositionsPanel> positions_panel_;
    std::shared_ptr<ui::TradesPanel> trades_panel_;
    std::shared_ptr<ui::StatusPanel> status_panel_;

    // Application state
    std::atomic<bool> running_;
    std::atomic<bool> shutdown_requested_;

public:
    TradingApplication() : running_(false), shutdown_requested_(false) {}

    bool initialize() {
        try {
            LOG_INFO("Initializing Trading System...");

            // Load configuration
            config_ = std::make_shared<Config>();
            if (!load_configuration()) {
                LOG_ERROR("Failed to load configuration");
                return false;
            }

            // Initialize logging
            initialize_logging();

            // Initialize core components
            if (!initialize_persistence()) {
                LOG_ERROR("Failed to initialize persistence service");
                return false;
            }

            if (!initialize_risk_management()) {
                LOG_ERROR("Failed to initialize risk management");
                return false;
            }

            if (!initialize_market_data()) {
                LOG_ERROR("Failed to initialize market data provider");
                return false;
            }

            if (!initialize_trading_engine()) {
                LOG_ERROR("Failed to initialize trading engine");
                return false;
            }

            // Initialize UI components
            if (!initialize_ui()) {
                LOG_ERROR("Failed to initialize UI");
                return false;
            }

            // Setup signal handlers
            setup_signal_handlers();

            LOG_INFO("Trading System initialized successfully");
            return true;

        } catch (const std::exception& e) {
            LOG_ERROR("Exception during initialization: {}", e.what());
            return false;
        }
    }

    void run() {
        if (!running_.load()) {
            LOG_ERROR("Application not properly initialized");
            return;
        }

        LOG_INFO("Starting Trading System...");

        try {
            // Connect to market data
            if (!market_data_provider_->connect()) {
                LOG_WARNING("Failed to connect to market data provider");
            }

            // Subscribe to default symbols
            auto default_symbols = config_->get_string_list("market_data.symbols", {"AAPL", "GOOGL", "MSFT"});
            for (const auto& symbol : default_symbols) {
                market_data_provider_->subscribe(symbol);
                LOG_INFO("Subscribed to {}", symbol);
            }

            // Start UI main loop
            LOG_INFO("Starting UI...");
            ui_manager_->run(); // This blocks until UI is closed

        } catch (const std::exception& e) {
            LOG_ERROR("Exception during execution: {}", e.what());
        }

        LOG_INFO("Trading System shutting down...");
    }

    void shutdown() {
        LOG_INFO("Shutdown requested");
        shutdown_requested_ = true;

        try {
            // Shutdown components in reverse order
            if (ui_manager_) {
                ui_manager_->shutdown();
            }

            if (market_data_provider_) {
                market_data_provider_->disconnect();
            }

            if (trading_engine_) {
                trading_engine_->shutdown();
            }

            if (persistence_) {
                persistence_->shutdown();
            }

            running_ = false;
            LOG_INFO("Trading System shutdown complete");

        } catch (const std::exception& e) {
            LOG_ERROR("Exception during shutdown: {}", e.what());
        }
    }

private:
    bool load_configuration() {
        try {
            // Load from default config file or create default configuration
            const std::string config_file = "config/trading_system.json";

            if (!config_->load_from_file(config_file)) {
                LOG_WARNING("Could not load config file, using defaults");
                create_default_configuration();
            }

            return true;
        } catch (const std::exception& e) {
            LOG_ERROR("Error loading configuration: {}", e.what());
            return false;
        }
    }

    void create_default_configuration() {
        // Market data configuration
        config_->set("market_data.simulation_mode", true);
        config_->set("market_data.websocket_url", "wss://api.example.com/v1/market_data");
        config_->set("market_data.symbols", std::vector<std::string>{"AAPL", "GOOGL", "MSFT", "TSLA", "AMZN"});
        config_->set("market_data.update_interval_ms", 100);

        // Risk management configuration
        config_->set("risk.max_position_size", 10000.0);
        config_->set("risk.max_order_size", 1000.0);
        config_->set("risk.max_daily_loss", 50000.0);
        config_->set("risk.enable_pre_trade_checks", true);

        // UI configuration
        config_->set("ui.theme", "dark");
        config_->set("ui.auto_sort", true);
        config_->set("ui.precision", 2);
        config_->set("ui.refresh_rate_ms", 100);

        // Persistence configuration
        config_->set("database.path", "./data/trading.db");
        config_->set("database.backup_path", "./data/backups/");
        config_->set("database.auto_backup", true);

        LOG_INFO("Created default configuration");
    }

    void initialize_logging() {
        auto log_level = config_->get_string("logging.level", "info");
        auto log_file = config_->get_string("logging.file", "logs/trading_system.log");

        Logger::initialize(log_level, log_file);
        LOG_INFO("Logging initialized: level={}, file={}", log_level, log_file);
    }

    bool initialize_persistence() {
        persistence_ = std::make_shared<PersistenceService>(config_);
        if (!persistence_->initialize()) {
            return false;
        }

        if (!persistence_->is_available()) {
            LOG_ERROR("Persistence service not available");
            return false;
        }

        LOG_INFO("Persistence service initialized: {}", persistence_->get_status());
        return true;
    }

    bool initialize_risk_management() {
        risk_manager_ = std::make_shared<RiskManager>(config_);
        if (!risk_manager_->initialize()) {
            return false;
        }

        LOG_INFO("Risk manager initialized");
        return true;
    }

    bool initialize_market_data() {
        market_data_provider_ = std::make_shared<MarketDataProvider>(config_);

        // Set up market data callbacks
        market_data_provider_->set_tick_callback([this](const MarketTick& tick) {
            // Update market data panel
            if (market_data_panel_) {
                std::vector<ui::MarketDataRow> data;
                ui::MarketDataRow row;
                row.symbol = tick.instrument_symbol;
                row.bid_price = tick.bid_price;
                row.ask_price = tick.ask_price;
                row.last_price = tick.last_price;
                row.spread = tick.get_spread();
                row.change_percent = 0.0; // Would be calculated from previous price
                row.last_update = tick.timestamp;
                row.is_stale = false;
                data.push_back(row);
                market_data_panel_->update_data(data);
            }

            // Update status panel
            if (status_panel_) {
                status_panel_->update_heartbeat();
            }
        });

        market_data_provider_->set_connection_callback([this](bool connected) {
            LOG_INFO("Market data connection status: {}", connected ? "Connected" : "Disconnected");
            if (status_panel_) {
                status_panel_->set_market_data_connected(connected);
            }
        });

        LOG_INFO("Market data provider initialized");
        return true;
    }

    bool initialize_trading_engine() {
        trading_engine_ = std::make_shared<TradingEngine>(config_, persistence_, risk_manager_);
        if (!trading_engine_->initialize()) {
            return false;
        }

        // Set up trading engine callbacks
        trading_engine_->set_order_update_callback([this](const ExecutionReport& report) {
            LOG_INFO("Order update: {} - {} -> {}", report.order_id,
                    order_status_to_string(report.old_status),
                    order_status_to_string(report.new_status));

            // Update UI panels
            update_order_displays();
        });

        trading_engine_->set_trade_callback([this](const Trade& trade) {
            LOG_INFO("Trade executed: {} {} {} @ {}", trade.trade_id,
                    trade.side == OrderSide::BUY ? "BUY" : "SELL",
                    trade.quantity, trade.price);

            // Update UI panels
            update_trade_displays();
        });

        trading_engine_->set_position_update_callback([this](const Position& position) {
            LOG_INFO("Position updated: {} - {} @ {}", position.instrument_symbol,
                    position.quantity, position.average_price);

            // Update UI panels
            update_position_displays();
        });

        LOG_INFO("Trading engine initialized");
        return true;
    }

    bool initialize_ui() {
        try {
            // Initialize UI manager
            ui_manager_ = std::make_shared<ui::UIManager>();
            if (!ui_manager_->initialize()) {
                return false;
            }

            // Create UI panels
            market_data_panel_ = std::make_shared<ui::MarketDataPanel>();
            order_entry_panel_ = std::make_shared<ui::OrderEntryPanel>();
            positions_panel_ = std::make_shared<ui::PositionsPanel>();
            trades_panel_ = std::make_shared<ui::TradesPanel>();
            status_panel_ = std::make_shared<ui::StatusPanel>();

            // Setup panel callbacks
            setup_ui_callbacks();

            running_ = true;
            LOG_INFO("UI initialized successfully");
            return true;

        } catch (const std::exception& e) {
            LOG_ERROR("UI initialization failed: {}", e.what());
            return false;
        }
    }

    void setup_ui_callbacks() {
        // Order entry callbacks
        order_entry_panel_->set_submit_callback([this](const ui::OrderFormData& form_data) {
            try {
                OrderRequest request;
                request.instrument_symbol = form_data.symbol;
                request.side = (form_data.side == "BUY") ? OrderSide::BUY : OrderSide::SELL;
                request.type = (form_data.type == "MARKET") ? OrderType::MARKET : OrderType::LIMIT;
                request.quantity = form_data.quantity;
                request.price = form_data.price;
                request.timestamp = std::chrono::system_clock::now();

                std::string order_id = trading_engine_->submit_order(request);
                if (!order_id.empty()) {
                    LOG_INFO("Order submitted: {}", order_id);
                    order_entry_panel_->reset_form();
                } else {
                    LOG_ERROR("Failed to submit order");
                }
            } catch (const std::exception& e) {
                LOG_ERROR("Error submitting order: {}", e.what());
            }
        });

        // Market data panel callbacks
        market_data_panel_->set_symbol_click_callback([this](const std::string& symbol) {
            order_entry_panel_->set_instrument(symbol);
            LOG_INFO("Selected symbol: {}", symbol);
        });

        market_data_panel_->set_subscribe_callback([this](const std::string& symbol) {
            if (market_data_provider_->subscribe(symbol)) {
                LOG_INFO("Subscribed to {}", symbol);
            } else {
                LOG_ERROR("Failed to subscribe to {}", symbol);
            }
        });

        // Position panel callbacks
        positions_panel_->set_position_click_callback([this](const std::string& symbol) {
            order_entry_panel_->set_instrument(symbol);
            LOG_INFO("Selected position: {}", symbol);
        });
    }

    void update_order_displays() {
        if (!ui_manager_) return;

        try {
            auto working_orders = trading_engine_->get_working_orders();
            std::vector<ui::OrderRow> order_rows;

            for (const auto& order : working_orders) {
                ui::OrderRow row;
                row.order_id = order->order_id;
                row.symbol = order->instrument_symbol;
                row.side = (order->side == OrderSide::BUY) ? "BUY" : "SELL";
                row.type = (order->type == OrderType::MARKET) ? "MARKET" : "LIMIT";
                row.status = order_status_to_string(order->status);
                row.quantity = order->quantity;
                row.price = order->price;
                row.filled_quantity = order->filled_quantity;
                row.remaining_quantity = order->remaining_quantity;
                row.created_time = order->created_time;
                row.last_modified = order->last_modified;
                order_rows.push_back(row);
            }

            // Update UI with order data
            // ui_manager_->update_orders(order_rows);

        } catch (const std::exception& e) {
            LOG_ERROR("Error updating order displays: {}", e.what());
        }
    }

    void update_trade_displays() {
        if (!trades_panel_) return;

        try {
            auto daily_trades = trading_engine_->get_daily_trades();
            std::vector<ui::TradeRow> trade_rows;

            for (const auto& trade : daily_trades) {
                ui::TradeRow row;
                row.trade_id = trade->trade_id;
                row.order_id = trade->order_id;
                row.symbol = trade->instrument_symbol;
                row.side = (trade->side == OrderSide::BUY) ? "BUY" : "SELL";
                row.quantity = trade->quantity;
                row.price = trade->price;
                row.notional_value = trade->get_notional_value();
                row.execution_time = trade->execution_time;
                trade_rows.push_back(row);
            }

            trades_panel_->update_data(trade_rows);

        } catch (const std::exception& e) {
            LOG_ERROR("Error updating trade displays: {}", e.what());
        }
    }

    void update_position_displays() {
        if (!positions_panel_) return;

        try {
            auto positions = trading_engine_->get_all_positions();
            std::vector<ui::PositionRow> position_rows;

            for (const auto& position : positions) {
                ui::PositionRow row;
                row.symbol = position->instrument_symbol;
                row.quantity = position->quantity;
                row.average_price = position->average_price;
                row.current_price = 0.0; // Would get from market data
                row.market_value = position->get_market_value(row.current_price);
                row.unrealized_pnl = position->unrealized_pnl;
                row.realized_pnl = position->realized_pnl;
                row.total_pnl = position->get_total_pnl(row.current_price);
                row.change_percent = 0.0; // Would calculate from current vs average price
                position_rows.push_back(row);
            }

            positions_panel_->update_data(position_rows);

        } catch (const std::exception& e) {
            LOG_ERROR("Error updating position displays: {}", e.what());
        }
    }

    void setup_signal_handlers() {
        // Set up signal handler for graceful shutdown
        signal(SIGINT, [](int) {
            std::cout << "\nShutdown signal received..." << std::endl;
            // Note: This is a simplified signal handler
            // In production, you'd want a more sophisticated approach
            exit(0);
        });

        signal(SIGTERM, [](int) {
            std::cout << "\nTermination signal received..." << std::endl;
            exit(0);
        });
    }

    std::string order_status_to_string(OrderStatus status) {
        switch (status) {
            case OrderStatus::NEW: return "NEW";
            case OrderStatus::ACCEPTED: return "ACCEPTED";
            case OrderStatus::PARTIALLY_FILLED: return "PARTIALLY_FILLED";
            case OrderStatus::FILLED: return "FILLED";
            case OrderStatus::CANCELED: return "CANCELED";
            case OrderStatus::REJECTED: return "REJECTED";
            default: return "UNKNOWN";
        }
    }
};

// Global application instance for signal handling
std::unique_ptr<TradingApplication> g_app;

void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down gracefully..." << std::endl;
    if (g_app) {
        g_app->shutdown();
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    std::cout << "=== C++ Trading System ===" << std::endl;
    std::cout << "Version: 1.0.0" << std::endl;
    std::cout << "Built: " << __DATE__ << " " << __TIME__ << std::endl;
    std::cout << std::endl;

    try {
        // Create application instance
        g_app = std::make_unique<TradingApplication>();

        // Set up signal handlers
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);

        // Initialize application
        if (!g_app->initialize()) {
            std::cerr << "Failed to initialize trading system" << std::endl;
            return 1;
        }

        // Run application
        g_app->run();

        // Clean shutdown
        g_app->shutdown();

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return 1;
    }

    std::cout << "Trading system shutdown complete" << std::endl;
    return 0;
}