#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include <string>

namespace trading {

/**
 * Centralized logging utility for the trading system
 * Uses spdlog for high-performance structured logging
 */
class Logger {
public:
    enum class Level {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        CRITICAL
    };

    // Initialize logging system
    static void initialize(const std::string& log_file_path = "logs/trading_system.log",
                          Level console_level = Level::INFO,
                          Level file_level = Level::DEBUG);

    // Get logger instances
    static std::shared_ptr<spdlog::logger> get_console_logger();
    static std::shared_ptr<spdlog::logger> get_file_logger();
    static std::shared_ptr<spdlog::logger> get_trade_logger();
    static std::shared_ptr<spdlog::logger> get_order_logger();
    static std::shared_ptr<spdlog::logger> get_market_data_logger();

    // Convenience logging functions
    static void trace(const std::string& message);
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);
    static void critical(const std::string& message);

    // Specialized logging for trading activities
    static void log_order(const std::string& order_id, const std::string& action, const std::string& details);
    static void log_trade(const std::string& trade_id, const std::string& order_id, const std::string& details);
    static void log_market_data(const std::string& symbol, const std::string& data);
    static void log_risk_event(const std::string& event, const std::string& details);

    // Set log levels
    static void set_console_level(Level level);
    static void set_file_level(Level level);

    // Shutdown logging
    static void shutdown();

private:
    static std::shared_ptr<spdlog::logger> console_logger_;
    static std::shared_ptr<spdlog::logger> file_logger_;
    static std::shared_ptr<spdlog::logger> trade_logger_;
    static std::shared_ptr<spdlog::logger> order_logger_;
    static std::shared_ptr<spdlog::logger> market_data_logger_;
    static bool initialized_;

    // Helper functions
    static spdlog::level::level_enum to_spdlog_level(Level level);
    static void ensure_directory_exists(const std::string& file_path);
};

// Convenience macros for logging
#define LOG_TRACE(msg) trading::Logger::trace(msg)
#define LOG_DEBUG(msg) trading::Logger::debug(msg)
#define LOG_INFO(msg) trading::Logger::info(msg)
#define LOG_WARN(msg) trading::Logger::warn(msg)
#define LOG_ERROR(msg) trading::Logger::error(msg)
#define LOG_CRITICAL(msg) trading::Logger::critical(msg)

#define LOG_ORDER(order_id, action, details) trading::Logger::log_order(order_id, action, details)
#define LOG_TRADE(trade_id, order_id, details) trading::Logger::log_trade(trade_id, order_id, details)
#define LOG_MARKET_DATA(symbol, data) trading::Logger::log_market_data(symbol, data)
#define LOG_RISK_EVENT(event, details) trading::Logger::log_risk_event(event, details)

} // namespace trading