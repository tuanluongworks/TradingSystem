#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <mutex>

namespace trading {

/**
 * Market Data Configuration
 */
struct MarketDataConfig {
    bool simulation_mode = true;
    std::string websocket_url = "wss://api.example.com/v1/market_data";
    std::vector<std::string> symbols = {"AAPL", "GOOGL", "MSFT", "TSLA", "AMZN"};
    int update_interval_ms = 100;

    // Validation
    bool is_valid() const;
    std::string get_validation_error() const;

    // JSON serialization
    void to_json(nlohmann::json& j) const;
    void from_json(const nlohmann::json& j);
};

/**
 * Risk Management Configuration
 */
struct RiskManagementConfig {
    double max_position_size = 10000.0;
    double max_order_size = 1000.0;
    double max_daily_loss = 50000.0;
    bool enable_risk_checks = true;

    // Position limits per symbol (optional)
    std::map<std::string, double> symbol_position_limits;
    std::map<std::string, double> symbol_order_limits;

    // Validation
    bool is_valid() const;
    std::string get_validation_error() const;

    // JSON serialization
    void to_json(nlohmann::json& j) const;
    void from_json(const nlohmann::json& j);
};

/**
 * UI Configuration
 */
struct UIConfig {
    std::string theme = "dark";
    bool auto_sort = true;
    int precision = 2;
    int refresh_rate_ms = 100;

    // Window layout
    bool show_market_data = true;
    bool show_order_entry = true;
    bool show_positions = true;
    bool show_trades = true;
    bool show_status = true;

    // Display options
    bool show_unrealized_pnl = true;
    bool use_dark_theme = false;

    // Limits
    int max_market_data_rows = 50;
    int max_order_history = 1000;
    int max_trade_history = 1000;

    // Update intervals
    int market_data_refresh = 100;
    int position_refresh = 500;
    int order_refresh = 250;

    // Validation
    bool is_valid() const;
    std::string get_validation_error() const;

    // JSON serialization
    void to_json(nlohmann::json& j) const;
    void from_json(const nlohmann::json& j);
};

/**
 * Persistence Configuration
 */
struct PersistenceConfig {
    std::string database_path = "./data/trading.db";
    std::string backup_path = "./data/backups/";
    bool auto_backup = true;
    int backup_interval_hours = 24;
    int max_backup_files = 7;

    // CSV export settings
    std::string csv_export_path = "./data/exports/";
    bool auto_export_trades = false;
    bool auto_export_orders = false;

    // Validation
    bool is_valid() const;
    std::string get_validation_error() const;

    // JSON serialization
    void to_json(nlohmann::json& j) const;
    void from_json(const nlohmann::json& j);
};

/**
 * Logging Configuration
 */
struct LoggingConfig {
    std::string log_level = "info";  // debug, info, warn, error
    std::string log_file_path = "./logs/trading_system.log";
    bool console_output = true;
    bool file_output = true;
    int max_file_size_mb = 100;
    int max_log_files = 10;

    // Validation
    bool is_valid() const;
    std::string get_validation_error() const;

    // JSON serialization
    void to_json(nlohmann::json& j) const;
    void from_json(const nlohmann::json& j);
};

/**
 * Complete Trading System Configuration
 */
struct TradingSystemConfig {
    MarketDataConfig market_data;
    RiskManagementConfig risk_management;
    UIConfig ui;
    PersistenceConfig persistence;
    LoggingConfig logging;

    // Application settings
    std::string application_name = "C++ Trading System";
    std::string version = "1.0.0";
    bool debug_mode = false;

    // Validation
    bool is_valid() const;
    std::string get_validation_error() const;

    // JSON serialization
    void to_json(nlohmann::json& j) const;
    void from_json(const nlohmann::json& j);
};

/**
 * Configuration Manager
 * Thread-safe configuration loading, saving, and management
 */
class ConfigurationManager {
public:
    explicit ConfigurationManager(const std::string& config_file_path = "config/trading_system.json");
    ~ConfigurationManager() = default;

    // Configuration loading/saving
    bool load_configuration();
    bool save_configuration() const;
    bool save_configuration(const std::string& file_path) const;

    // Configuration access (thread-safe)
    TradingSystemConfig get_configuration() const;
    MarketDataConfig get_market_data_config() const;
    RiskManagementConfig get_risk_management_config() const;
    UIConfig get_ui_config() const;
    PersistenceConfig get_persistence_config() const;
    LoggingConfig get_logging_config() const;

    // Configuration updates (thread-safe)
    bool update_market_data_config(const MarketDataConfig& config);
    bool update_risk_management_config(const RiskManagementConfig& config);
    bool update_ui_config(const UIConfig& config);
    bool update_persistence_config(const PersistenceConfig& config);
    bool update_logging_config(const LoggingConfig& config);

    // Utility methods
    bool validate_configuration() const;
    std::string get_validation_errors() const;

    // File operations
    bool backup_configuration(const std::string& backup_path) const;
    bool restore_configuration(const std::string& backup_path);

    // Default configuration
    static TradingSystemConfig get_default_configuration();
    bool reset_to_defaults();

    // Configuration file management
    bool create_default_config_file() const;
    std::string get_config_file_path() const { return config_file_path_; }
    bool config_file_exists() const;

    // Environment variable support
    void load_from_environment();
    std::optional<std::string> get_env_variable(const std::string& var_name) const;

private:
    std::string config_file_path_;
    mutable std::mutex config_mutex_;
    TradingSystemConfig current_config_;
    bool is_loaded_;

    // Helper methods
    bool validate_file_path(const std::string& path) const;
    bool ensure_directory_exists(const std::string& path) const;
    void apply_environment_overrides();

    // JSON helpers
    nlohmann::json config_to_json(const TradingSystemConfig& config) const;
    TradingSystemConfig json_to_config(const nlohmann::json& j) const;

    // Error handling
    void log_error(const std::string& operation, const std::string& error) const;
    void log_warning(const std::string& operation, const std::string& warning) const;
    void log_info(const std::string& operation, const std::string& info) const;
};

// Global configuration access (singleton pattern)
class GlobalConfig {
public:
    static ConfigurationManager& instance();
    static TradingSystemConfig get();
    static void set_config_path(const std::string& path);

private:
    static std::unique_ptr<ConfigurationManager> instance_;
    static std::once_flag init_flag_;
    static void initialize();
};

// Convenience macros for configuration access
#define TRADING_CONFIG() GlobalConfig::get()
#define MARKET_DATA_CONFIG() GlobalConfig::get().market_data
#define RISK_CONFIG() GlobalConfig::get().risk_management
#define UI_CONFIG() GlobalConfig::get().ui
#define PERSISTENCE_CONFIG() GlobalConfig::get().persistence
#define LOGGING_CONFIG() GlobalConfig::get().logging

} // namespace trading