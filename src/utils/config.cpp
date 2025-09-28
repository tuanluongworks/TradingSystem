#include "config.hpp"
#include "logging.hpp"
#include "exceptions.hpp"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstdlib>

namespace trading {

// MarketDataConfig implementation
bool MarketDataConfig::is_valid() const {
    if (symbols.empty()) return false;
    if (update_interval_ms < 10 || update_interval_ms > 10000) return false;
    if (!simulation_mode && websocket_url.empty()) return false;
    return true;
}

std::string MarketDataConfig::get_validation_error() const {
    if (symbols.empty()) return "No symbols configured for market data";
    if (update_interval_ms < 10) return "Update interval too low (minimum 10ms)";
    if (update_interval_ms > 10000) return "Update interval too high (maximum 10000ms)";
    if (!simulation_mode && websocket_url.empty()) return "WebSocket URL required for live data";
    return "";
}

void MarketDataConfig::to_json(nlohmann::json& j) const {
    j = nlohmann::json{
        {"simulation_mode", simulation_mode},
        {"websocket_url", websocket_url},
        {"symbols", symbols},
        {"update_interval_ms", update_interval_ms}
    };
}

void MarketDataConfig::from_json(const nlohmann::json& j) {
    simulation_mode = j.value("simulation_mode", true);
    websocket_url = j.value("websocket_url", "wss://api.example.com/v1/market_data");
    symbols = j.value("symbols", std::vector<std::string>{"AAPL", "GOOGL", "MSFT", "TSLA", "AMZN"});
    update_interval_ms = j.value("update_interval_ms", 100);
}

// RiskManagementConfig implementation
bool RiskManagementConfig::is_valid() const {
    if (max_position_size <= 0) return false;
    if (max_order_size <= 0) return false;
    if (max_daily_loss <= 0) return false;
    if (max_order_size > max_position_size) return false;
    return true;
}

std::string RiskManagementConfig::get_validation_error() const {
    if (max_position_size <= 0) return "Max position size must be positive";
    if (max_order_size <= 0) return "Max order size must be positive";
    if (max_daily_loss <= 0) return "Max daily loss must be positive";
    if (max_order_size > max_position_size) return "Max order size cannot exceed max position size";
    return "";
}

void RiskManagementConfig::to_json(nlohmann::json& j) const {
    j = nlohmann::json{
        {"max_position_size", max_position_size},
        {"max_order_size", max_order_size},
        {"max_daily_loss", max_daily_loss},
        {"enable_risk_checks", enable_risk_checks},
        {"symbol_position_limits", symbol_position_limits},
        {"symbol_order_limits", symbol_order_limits}
    };
}

void RiskManagementConfig::from_json(const nlohmann::json& j) {
    max_position_size = j.value("max_position_size", 10000.0);
    max_order_size = j.value("max_order_size", 1000.0);
    max_daily_loss = j.value("max_daily_loss", 50000.0);
    enable_risk_checks = j.value("enable_risk_checks", true);

    if (j.contains("symbol_position_limits")) {
        symbol_position_limits = j["symbol_position_limits"];
    }
    if (j.contains("symbol_order_limits")) {
        symbol_order_limits = j["symbol_order_limits"];
    }
}

// UIConfig implementation
bool UIConfig::is_valid() const {
    if (precision < 0 || precision > 8) return false;
    if (refresh_rate_ms < 16 || refresh_rate_ms > 5000) return false;
    if (max_market_data_rows < 10 || max_market_data_rows > 1000) return false;
    if (max_order_history < 100 || max_order_history > 100000) return false;
    if (max_trade_history < 100 || max_trade_history > 100000) return false;
    return true;
}

std::string UIConfig::get_validation_error() const {
    if (precision < 0 || precision > 8) return "Precision must be between 0 and 8";
    if (refresh_rate_ms < 16) return "Refresh rate too high (minimum 16ms for 60fps)";
    if (refresh_rate_ms > 5000) return "Refresh rate too low (maximum 5000ms)";
    if (max_market_data_rows < 10) return "Too few market data rows (minimum 10)";
    if (max_market_data_rows > 1000) return "Too many market data rows (maximum 1000)";
    if (max_order_history < 100) return "Order history too small (minimum 100)";
    if (max_order_history > 100000) return "Order history too large (maximum 100000)";
    return "";
}

void UIConfig::to_json(nlohmann::json& j) const {
    j = nlohmann::json{
        {"theme", theme},
        {"auto_sort", auto_sort},
        {"precision", precision},
        {"refresh_rate_ms", refresh_rate_ms},
        {"show_market_data", show_market_data},
        {"show_order_entry", show_order_entry},
        {"show_positions", show_positions},
        {"show_trades", show_trades},
        {"show_status", show_status},
        {"show_unrealized_pnl", show_unrealized_pnl},
        {"use_dark_theme", use_dark_theme},
        {"max_market_data_rows", max_market_data_rows},
        {"max_order_history", max_order_history},
        {"max_trade_history", max_trade_history},
        {"market_data_refresh", market_data_refresh},
        {"position_refresh", position_refresh},
        {"order_refresh", order_refresh}
    };
}

void UIConfig::from_json(const nlohmann::json& j) {
    theme = j.value("theme", "dark");
    auto_sort = j.value("auto_sort", true);
    precision = j.value("precision", 2);
    refresh_rate_ms = j.value("refresh_rate_ms", 100);
    show_market_data = j.value("show_market_data", true);
    show_order_entry = j.value("show_order_entry", true);
    show_positions = j.value("show_positions", true);
    show_trades = j.value("show_trades", true);
    show_status = j.value("show_status", true);
    show_unrealized_pnl = j.value("show_unrealized_pnl", true);
    use_dark_theme = j.value("use_dark_theme", false);
    max_market_data_rows = j.value("max_market_data_rows", 50);
    max_order_history = j.value("max_order_history", 1000);
    max_trade_history = j.value("max_trade_history", 1000);
    market_data_refresh = j.value("market_data_refresh", 100);
    position_refresh = j.value("position_refresh", 500);
    order_refresh = j.value("order_refresh", 250);
}

// PersistenceConfig implementation
bool PersistenceConfig::is_valid() const {
    if (database_path.empty()) return false;
    if (backup_path.empty()) return false;
    if (backup_interval_hours < 1 || backup_interval_hours > 168) return false;
    if (max_backup_files < 1 || max_backup_files > 100) return false;
    return true;
}

std::string PersistenceConfig::get_validation_error() const {
    if (database_path.empty()) return "Database path cannot be empty";
    if (backup_path.empty()) return "Backup path cannot be empty";
    if (backup_interval_hours < 1) return "Backup interval too frequent (minimum 1 hour)";
    if (backup_interval_hours > 168) return "Backup interval too long (maximum 168 hours/1 week)";
    if (max_backup_files < 1) return "Must keep at least 1 backup file";
    if (max_backup_files > 100) return "Too many backup files (maximum 100)";
    return "";
}

void PersistenceConfig::to_json(nlohmann::json& j) const {
    j = nlohmann::json{
        {"database_path", database_path},
        {"backup_path", backup_path},
        {"auto_backup", auto_backup},
        {"backup_interval_hours", backup_interval_hours},
        {"max_backup_files", max_backup_files},
        {"csv_export_path", csv_export_path},
        {"auto_export_trades", auto_export_trades},
        {"auto_export_orders", auto_export_orders}
    };
}

void PersistenceConfig::from_json(const nlohmann::json& j) {
    database_path = j.value("database_path", "./data/trading.db");
    backup_path = j.value("backup_path", "./data/backups/");
    auto_backup = j.value("auto_backup", true);
    backup_interval_hours = j.value("backup_interval_hours", 24);
    max_backup_files = j.value("max_backup_files", 7);
    csv_export_path = j.value("csv_export_path", "./data/exports/");
    auto_export_trades = j.value("auto_export_trades", false);
    auto_export_orders = j.value("auto_export_orders", false);
}

// LoggingConfig implementation
bool LoggingConfig::is_valid() const {
    std::vector<std::string> valid_levels = {"debug", "info", "warn", "error"};
    if (std::find(valid_levels.begin(), valid_levels.end(), log_level) == valid_levels.end()) {
        return false;
    }
    if (max_file_size_mb < 1 || max_file_size_mb > 1000) return false;
    if (max_log_files < 1 || max_log_files > 100) return false;
    return true;
}

std::string LoggingConfig::get_validation_error() const {
    std::vector<std::string> valid_levels = {"debug", "info", "warn", "error"};
    if (std::find(valid_levels.begin(), valid_levels.end(), log_level) == valid_levels.end()) {
        return "Invalid log level. Must be: debug, info, warn, or error";
    }
    if (max_file_size_mb < 1) return "Log file size too small (minimum 1MB)";
    if (max_file_size_mb > 1000) return "Log file size too large (maximum 1000MB)";
    if (max_log_files < 1) return "Must keep at least 1 log file";
    if (max_log_files > 100) return "Too many log files (maximum 100)";
    return "";
}

void LoggingConfig::to_json(nlohmann::json& j) const {
    j = nlohmann::json{
        {"log_level", log_level},
        {"log_file_path", log_file_path},
        {"console_output", console_output},
        {"file_output", file_output},
        {"max_file_size_mb", max_file_size_mb},
        {"max_log_files", max_log_files}
    };
}

void LoggingConfig::from_json(const nlohmann::json& j) {
    log_level = j.value("log_level", "info");
    log_file_path = j.value("log_file_path", "./logs/trading_system.log");
    console_output = j.value("console_output", true);
    file_output = j.value("file_output", true);
    max_file_size_mb = j.value("max_file_size_mb", 100);
    max_log_files = j.value("max_log_files", 10);
}

// TradingSystemConfig implementation
bool TradingSystemConfig::is_valid() const {
    return market_data.is_valid() &&
           risk_management.is_valid() &&
           ui.is_valid() &&
           persistence.is_valid() &&
           logging.is_valid();
}

std::string TradingSystemConfig::get_validation_error() const {
    std::string error;

    if (!market_data.is_valid()) {
        error += "Market Data: " + market_data.get_validation_error() + "; ";
    }
    if (!risk_management.is_valid()) {
        error += "Risk Management: " + risk_management.get_validation_error() + "; ";
    }
    if (!ui.is_valid()) {
        error += "UI: " + ui.get_validation_error() + "; ";
    }
    if (!persistence.is_valid()) {
        error += "Persistence: " + persistence.get_validation_error() + "; ";
    }
    if (!logging.is_valid()) {
        error += "Logging: " + logging.get_validation_error() + "; ";
    }

    return error;
}

void TradingSystemConfig::to_json(nlohmann::json& j) const {
    nlohmann::json market_data_json, risk_json, ui_json, persistence_json, logging_json;

    market_data.to_json(market_data_json);
    risk_management.to_json(risk_json);
    ui.to_json(ui_json);
    persistence.to_json(persistence_json);
    logging.to_json(logging_json);

    j = nlohmann::json{
        {"application_name", application_name},
        {"version", version},
        {"debug_mode", debug_mode},
        {"market_data", market_data_json},
        {"risk_management", risk_json},
        {"ui", ui_json},
        {"persistence", persistence_json},
        {"logging", logging_json}
    };
}

void TradingSystemConfig::from_json(const nlohmann::json& j) {
    application_name = j.value("application_name", "C++ Trading System");
    version = j.value("version", "1.0.0");
    debug_mode = j.value("debug_mode", false);

    if (j.contains("market_data")) {
        market_data.from_json(j["market_data"]);
    }
    if (j.contains("risk_management")) {
        risk_management.from_json(j["risk_management"]);
    }
    if (j.contains("ui")) {
        ui.from_json(j["ui"]);
    }
    if (j.contains("persistence")) {
        persistence.from_json(j["persistence"]);
    }
    if (j.contains("logging")) {
        logging.from_json(j["logging"]);
    }
}

// ConfigurationManager implementation
ConfigurationManager::ConfigurationManager(const std::string& config_file_path)
    : config_file_path_(config_file_path)
    , is_loaded_(false) {
}

bool ConfigurationManager::load_configuration() {
    std::lock_guard<std::mutex> lock(config_mutex_);

    try {
        if (!config_file_exists()) {
            log_info("load_configuration", "Config file not found, creating default: " + config_file_path_);
            current_config_ = get_default_configuration();
            return create_default_config_file();
        }

        std::ifstream file(config_file_path_);
        if (!file.is_open()) {
            log_error("load_configuration", "Failed to open config file: " + config_file_path_);
            return false;
        }

        nlohmann::json j;
        file >> j;
        file.close();

        current_config_ = json_to_config(j);

        // Apply environment variable overrides
        apply_environment_overrides();

        if (!current_config_.is_valid()) {
            log_error("load_configuration", "Invalid configuration: " + current_config_.get_validation_error());
            return false;
        }

        is_loaded_ = true;
        log_info("load_configuration", "Configuration loaded successfully from: " + config_file_path_);
        return true;

    } catch (const nlohmann::json::exception& e) {
        log_error("load_configuration", "JSON parsing error: " + std::string(e.what()));
        return false;
    } catch (const std::exception& e) {
        log_error("load_configuration", "Unexpected error: " + std::string(e.what()));
        return false;
    }
}

bool ConfigurationManager::save_configuration() const {
    return save_configuration(config_file_path_);
}

bool ConfigurationManager::save_configuration(const std::string& file_path) const {
    std::lock_guard<std::mutex> lock(config_mutex_);

    try {
        // Ensure directory exists
        std::filesystem::path config_path(file_path);
        if (config_path.has_parent_path()) {
            ensure_directory_exists(config_path.parent_path().string());
        }

        nlohmann::json j = config_to_json(current_config_);

        std::ofstream file(file_path);
        if (!file.is_open()) {
            log_error("save_configuration", "Failed to open file for writing: " + file_path);
            return false;
        }

        file << j.dump(4); // Pretty print with 4-space indentation
        file.close();

        log_info("save_configuration", "Configuration saved to: " + file_path);
        return true;

    } catch (const std::exception& e) {
        log_error("save_configuration", "Error saving configuration: " + std::string(e.what()));
        return false;
    }
}

TradingSystemConfig ConfigurationManager::get_configuration() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return current_config_;
}

MarketDataConfig ConfigurationManager::get_market_data_config() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return current_config_.market_data;
}

RiskManagementConfig ConfigurationManager::get_risk_management_config() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return current_config_.risk_management;
}

UIConfig ConfigurationManager::get_ui_config() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return current_config_.ui;
}

PersistenceConfig ConfigurationManager::get_persistence_config() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return current_config_.persistence;
}

LoggingConfig ConfigurationManager::get_logging_config() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return current_config_.logging;
}

bool ConfigurationManager::update_market_data_config(const MarketDataConfig& config) {
    if (!config.is_valid()) {
        log_error("update_market_data_config", "Invalid configuration: " + config.get_validation_error());
        return false;
    }

    std::lock_guard<std::mutex> lock(config_mutex_);
    current_config_.market_data = config;
    return true;
}

bool ConfigurationManager::update_risk_management_config(const RiskManagementConfig& config) {
    if (!config.is_valid()) {
        log_error("update_risk_management_config", "Invalid configuration: " + config.get_validation_error());
        return false;
    }

    std::lock_guard<std::mutex> lock(config_mutex_);
    current_config_.risk_management = config;
    return true;
}

bool ConfigurationManager::update_ui_config(const UIConfig& config) {
    if (!config.is_valid()) {
        log_error("update_ui_config", "Invalid configuration: " + config.get_validation_error());
        return false;
    }

    std::lock_guard<std::mutex> lock(config_mutex_);
    current_config_.ui = config;
    return true;
}

bool ConfigurationManager::update_persistence_config(const PersistenceConfig& config) {
    if (!config.is_valid()) {
        log_error("update_persistence_config", "Invalid configuration: " + config.get_validation_error());
        return false;
    }

    std::lock_guard<std::mutex> lock(config_mutex_);
    current_config_.persistence = config;
    return true;
}

bool ConfigurationManager::update_logging_config(const LoggingConfig& config) {
    if (!config.is_valid()) {
        log_error("update_logging_config", "Invalid configuration: " + config.get_validation_error());
        return false;
    }

    std::lock_guard<std::mutex> lock(config_mutex_);
    current_config_.logging = config;
    return true;
}

bool ConfigurationManager::validate_configuration() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return current_config_.is_valid();
}

std::string ConfigurationManager::get_validation_errors() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return current_config_.get_validation_error();
}

bool ConfigurationManager::backup_configuration(const std::string& backup_path) const {
    return save_configuration(backup_path);
}

bool ConfigurationManager::restore_configuration(const std::string& backup_path) {
    if (!std::filesystem::exists(backup_path)) {
        log_error("restore_configuration", "Backup file does not exist: " + backup_path);
        return false;
    }

    std::string original_path = config_file_path_;
    config_file_path_ = backup_path;

    bool success = load_configuration();

    config_file_path_ = original_path;

    if (success) {
        save_configuration(); // Save restored config to original location
    }

    return success;
}

TradingSystemConfig ConfigurationManager::get_default_configuration() {
    return TradingSystemConfig{}; // Uses default values from struct initialization
}

bool ConfigurationManager::reset_to_defaults() {
    std::lock_guard<std::mutex> lock(config_mutex_);
    current_config_ = get_default_configuration();
    is_loaded_ = true;
    return save_configuration();
}

bool ConfigurationManager::create_default_config_file() const {
    TradingSystemConfig default_config = get_default_configuration();
    nlohmann::json j = config_to_json(default_config);

    try {
        // Ensure directory exists
        std::filesystem::path config_path(config_file_path_);
        if (config_path.has_parent_path()) {
            ensure_directory_exists(config_path.parent_path().string());
        }

        std::ofstream file(config_file_path_);
        if (!file.is_open()) {
            log_error("create_default_config_file", "Failed to create config file: " + config_file_path_);
            return false;
        }

        file << j.dump(4); // Pretty print
        file.close();

        log_info("create_default_config_file", "Default configuration created: " + config_file_path_);
        return true;

    } catch (const std::exception& e) {
        log_error("create_default_config_file", "Error creating default config: " + std::string(e.what()));
        return false;
    }
}

bool ConfigurationManager::config_file_exists() const {
    return std::filesystem::exists(config_file_path_);
}

void ConfigurationManager::load_from_environment() {
    apply_environment_overrides();
}

std::optional<std::string> ConfigurationManager::get_env_variable(const std::string& var_name) const {
    const char* value = std::getenv(var_name.c_str());
    if (value) {
        return std::string(value);
    }
    return std::nullopt;
}

// Helper methods

bool ConfigurationManager::validate_file_path(const std::string& path) const {
    std::filesystem::path fs_path(path);
    return fs_path.is_absolute() || fs_path.is_relative();
}

bool ConfigurationManager::ensure_directory_exists(const std::string& path) const {
    try {
        return std::filesystem::create_directories(path);
    } catch (const std::exception& e) {
        log_error("ensure_directory_exists", "Failed to create directory " + path + ": " + e.what());
        return false;
    }
}

void ConfigurationManager::apply_environment_overrides() {
    // Market data overrides
    if (auto url = get_env_variable("TRADING_WEBSOCKET_URL")) {
        current_config_.market_data.websocket_url = *url;
        current_config_.market_data.simulation_mode = false;
    }
    if (auto sim_mode = get_env_variable("TRADING_SIMULATION_MODE")) {
        current_config_.market_data.simulation_mode = (*sim_mode == "true" || *sim_mode == "1");
    }

    // Risk management overrides
    if (auto max_pos = get_env_variable("TRADING_MAX_POSITION_SIZE")) {
        try {
            current_config_.risk_management.max_position_size = std::stod(*max_pos);
        } catch (const std::exception& e) {
            log_warning("apply_environment_overrides", "Invalid TRADING_MAX_POSITION_SIZE value: " + *max_pos);
        }
    }

    // Database path override
    if (auto db_path = get_env_variable("TRADING_DATABASE_PATH")) {
        current_config_.persistence.database_path = *db_path;
    }

    // Log level override
    if (auto log_level = get_env_variable("TRADING_LOG_LEVEL")) {
        current_config_.logging.log_level = *log_level;
    }

    // Debug mode override
    if (auto debug = get_env_variable("TRADING_DEBUG_MODE")) {
        current_config_.debug_mode = (*debug == "true" || *debug == "1");
    }
}

nlohmann::json ConfigurationManager::config_to_json(const TradingSystemConfig& config) const {
    nlohmann::json j;
    config.to_json(j);
    return j;
}

TradingSystemConfig ConfigurationManager::json_to_config(const nlohmann::json& j) const {
    TradingSystemConfig config;
    config.from_json(j);
    return config;
}

void ConfigurationManager::log_error(const std::string& operation, const std::string& error) const {
    std::string msg = "ConfigurationManager::" + operation + " - " + error;
    TRADING_LOG_ERROR(msg);
}

void ConfigurationManager::log_warning(const std::string& operation, const std::string& warning) const {
    std::string msg = "ConfigurationManager::" + operation + " - " + warning;
    TRADING_LOG_WARN(msg);
}

void ConfigurationManager::log_info(const std::string& operation, const std::string& info) const {
    std::string msg = "ConfigurationManager::" + operation + " - " + info;
    TRADING_LOG_INFO(msg);
}

// GlobalConfig implementation

std::unique_ptr<ConfigurationManager> GlobalConfig::instance_;
std::once_flag GlobalConfig::init_flag_;

ConfigurationManager& GlobalConfig::instance() {
    std::call_once(init_flag_, initialize);
    return *instance_;
}

TradingSystemConfig GlobalConfig::get() {
    return instance().get_configuration();
}

void GlobalConfig::set_config_path(const std::string& path) {
    instance_ = std::make_unique<ConfigurationManager>(path);
    instance_->load_configuration();
}

void GlobalConfig::initialize() {
    instance_ = std::make_unique<ConfigurationManager>();
    instance_->load_configuration();
}

} // namespace trading