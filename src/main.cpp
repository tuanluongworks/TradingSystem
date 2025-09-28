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
    std::shared_ptr<ConfigurationManager> config_manager_;
    TradingSystemConfig config_;
    std::shared_ptr<SQLiteService> persistence_;
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
            config_manager_ = std::make_shared<ConfigurationManager>();
            config_ = config_manager_->get_configuration();
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
            TRADING_LOG_ERROR("Exception during initialization: {}", e.what());
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
                TRADING_LOG_WARN("Failed to connect to market data provider");
            }

            // Subscribe to default symbols
            auto default_symbols = config_.market_data.symbols;
            for (const auto& symbol : default_symbols) {
                market_data_provider_->subscribe(symbol);
                TRADING_LOG_INFO("Subscribed to {}", symbol);
            }

            // Start UI main loop
            LOG_INFO("Starting UI...");
            ui_manager_->run(); // This blocks until UI is closed

        } catch (const std::exception& e) {
            TRADING_LOG_ERROR("Exception during execution: {}", e.what());
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
                persistence_->close();
            }

            running_ = false;
            LOG_INFO("Trading System shutdown complete");

        } catch (const std::exception& e) {
            TRADING_LOG_ERROR("Exception during shutdown: {}", e.what());
        }
    }

private:
    bool load_configuration() {
        try {
            // Load from default config file or create default configuration
            const std::string config_file = "config/trading_system.json";

            if (!config_manager_->load_configuration()) {
                TRADING_LOG_WARN("Could not load config file, using defaults");
                create_default_configuration();
            }
            config_ = config_manager_->get_configuration();

            return true;
        } catch (const std::exception& e) {
            TRADING_LOG_ERROR("Error loading configuration: {}", e.what());
            return false;
        }
    }

    void create_default_configuration() {
        // Use default TradingSystemConfig which already has sensible defaults
        // The configuration manager will save these defaults
        config_manager_->save_configuration();
        config_ = config_manager_->get_configuration();
        TRADING_LOG_INFO("Created default configuration");
    }

    void initialize_logging() {
        auto log_config = config_.logging;

        Logger::initialize(log_config.log_file_path);
        TRADING_LOG_INFO("Logging initialized: level={}, file={}",
                        log_config.log_level, log_config.log_file_path);
    }

    bool initialize_persistence() {
        persistence_ = std::make_shared<SQLiteService>(config_.persistence.database_path);
        if (!persistence_->initialize()) {
            return false;
        }

        if (!persistence_->is_available()) {
            TRADING_LOG_ERROR("Persistence service not available");
            return false;
        }

        TRADING_LOG_INFO("Persistence service initialized: {}", persistence_->get_status());
        return true;
    }

    bool initialize_risk_management() {
        risk_manager_ = std::make_shared<RiskManager>(config_.risk_management);

        TRADING_LOG_INFO("Risk manager initialized");
        return true;
    }

    bool initialize_market_data() {
        // Convert MarketDataConfig to ProviderConfig
        MarketDataProvider::ProviderConfig provider_config;
        provider_config.mode = config_.market_data.simulation_mode ?
                              MarketDataProvider::ProviderMode::SIMULATION :
                              MarketDataProvider::ProviderMode::WEBSOCKET;
        provider_config.websocket_url = config_.market_data.websocket_url;
        provider_config.update_interval_ms = config_.market_data.update_interval_ms;
        provider_config.default_symbols = config_.market_data.symbols;

        market_data_provider_ = std::make_shared<MarketDataProvider>(provider_config);

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
            TRADING_LOG_INFO("Market data connection status: {}", connected ? "Connected" : "Disconnected");
            if (status_panel_) {
                status_panel_->set_market_data_connected(connected);
            }
        });

        TRADING_LOG_INFO("Market data provider initialized");
        return true;
    }

    bool initialize_trading_engine() {
        trading_engine_ = std::make_shared<TradingEngine>(risk_manager_, persistence_);
        if (!trading_engine_->initialize()) {
            return false;
        }

        // Set up trading engine callbacks
        trading_engine_->set_order_update_callback([this](const ExecutionReport& report) {
            TRADING_LOG_INFO("Order update: {} - {} -> {}", report.order_id,
                    order_status_to_string(report.old_status),
                    order_status_to_string(report.new_status));

            // Update UI panels
            update_order_displays();
        });

        trading_engine_->set_trade_callback([this](const Trade& trade) {
            TRADING_LOG_INFO("Trade executed: {} {} {} @ {}", trade.get_trade_id(),
                    trade.get_side() == OrderSide::BUY ? "BUY" : "SELL",
                    trade.get_quantity(), trade.get_price());

            // Update UI panels
            update_trade_displays();
        });

        trading_engine_->set_position_update_callback([this](const Position& position) {
            TRADING_LOG_INFO("Position updated: {} - {} @ {}", position.get_instrument_symbol(),
                    position.get_quantity(), position.get_average_price());

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
            TRADING_LOG_ERROR("UI initialization failed: {}", e.what());
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
                    TRADING_LOG_INFO("Order submitted: {}", order_id);
                    order_entry_panel_->reset_form();
                } else {
                    LOG_ERROR("Failed to submit order");
                }
            } catch (const std::exception& e) {
                TRADING_LOG_ERROR("Error submitting order: {}", e.what());
            }
        });

        // Market data panel callbacks
        market_data_panel_->set_symbol_click_callback([this](const std::string& symbol) {
            order_entry_panel_->set_instrument(symbol);
            TRADING_LOG_INFO("Selected symbol: {}", symbol);
        });

        market_data_panel_->set_subscribe_callback([this](const std::string& symbol) {
            if (market_data_provider_->subscribe(symbol)) {
                TRADING_LOG_INFO("Subscribed to {}", symbol);
            } else {
                TRADING_LOG_ERROR("Failed to subscribe to {}", symbol);
            }
        });

        // Position panel callbacks
        positions_panel_->set_position_click_callback([this](const std::string& symbol) {
            order_entry_panel_->set_instrument(symbol);
            TRADING_LOG_INFO("Selected position: {}", symbol);
        });
    }

    void update_order_displays() {
        if (!ui_manager_) return;

        try {
            auto working_orders = trading_engine_->get_working_orders();
            std::vector<ui::OrderRow> order_rows;

            for (const auto& order : working_orders) {
                ui::OrderRow row;
                row.order_id = order->get_order_id();
                row.symbol = order->get_instrument_symbol();
                row.side = (order->get_side() == OrderSide::BUY) ? "BUY" : "SELL";
                row.type = (order->get_type() == OrderType::MARKET) ? "MARKET" : "LIMIT";
                row.status = order_status_to_string(order->get_status());
                row.quantity = order->get_quantity();
                row.price = order->get_price();
                row.filled_quantity = order->get_filled_quantity();
                row.remaining_quantity = order->get_remaining_quantity();
                row.created_time = order->get_created_time();
                row.last_modified = order->get_last_modified();
                order_rows.push_back(row);
            }

            // Update UI with order data
            // ui_manager_->update_orders(order_rows);

        } catch (const std::exception& e) {
            TRADING_LOG_ERROR("Error updating order displays: {}", e.what());
        }
    }

    void update_trade_displays() {
        if (!trades_panel_) return;

        try {
            auto daily_trades = trading_engine_->get_daily_trades();
            std::vector<ui::TradeRow> trade_rows;

            for (const auto& trade : daily_trades) {
                ui::TradeRow row;
                row.trade_id = trade->get_trade_id();
                row.order_id = trade->get_order_id();
                row.symbol = trade->get_instrument_symbol();
                row.side = (trade->get_side() == OrderSide::BUY) ? "BUY" : "SELL";
                row.quantity = trade->get_quantity();
                row.price = trade->get_price();
                row.notional_value = trade->get_notional_value();
                row.execution_time = trade->get_execution_time();
                trade_rows.push_back(row);
            }

            trades_panel_->update_data(trade_rows);

        } catch (const std::exception& e) {
            TRADING_LOG_ERROR("Error updating trade displays: {}", e.what());
        }
    }

    void update_position_displays() {
        if (!positions_panel_) return;

        try {
            auto positions = trading_engine_->get_all_positions();
            std::vector<ui::PositionRow> position_rows;

            for (const auto& position : positions) {
                ui::PositionRow row;
                row.symbol = position->get_instrument_symbol();
                row.quantity = position->get_quantity();
                row.average_price = position->get_average_price();
                row.current_price = 0.0; // Would get from market data
                row.market_value = position->get_market_value(row.current_price);
                row.unrealized_pnl = position->get_unrealized_pnl();
                row.realized_pnl = position->get_realized_pnl();
                row.total_pnl = position->get_total_pnl(row.current_price);
                row.change_percent = 0.0; // Would calculate from current vs average price
                position_rows.push_back(row);
            }

            positions_panel_->update_data(position_rows);

        } catch (const std::exception& e) {
            TRADING_LOG_ERROR("Error updating position displays: {}", e.what());
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

int main(int /*argc*/, char* /*argv*/[]) {
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