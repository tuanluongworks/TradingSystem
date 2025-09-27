#include "logging.hpp"
#include <spdlog/sinks/rotating_file_sink.h>
#include <filesystem>
#include <iostream>

namespace trading {

// Static member definitions
std::shared_ptr<spdlog::logger> Logger::console_logger_;
std::shared_ptr<spdlog::logger> Logger::file_logger_;
std::shared_ptr<spdlog::logger> Logger::trade_logger_;
std::shared_ptr<spdlog::logger> Logger::order_logger_;
std::shared_ptr<spdlog::logger> Logger::market_data_logger_;
bool Logger::initialized_ = false;

void Logger::initialize(const std::string& log_file_path,
                       Level console_level,
                       Level file_level) {
    if (initialized_) {
        return;
    }

    try {
        // Ensure log directory exists
        ensure_directory_exists(log_file_path);

        // Create console sink
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(to_spdlog_level(console_level));
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");

        // Create rotating file sink (100MB files, 10 files max)
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            log_file_path, 1024 * 1024 * 100, 10);
        file_sink->set_level(to_spdlog_level(file_level));
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] [%t] %v");

        // Create main loggers
        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};

        console_logger_ = std::make_shared<spdlog::logger>("console", console_sink);
        file_logger_ = std::make_shared<spdlog::logger>("file", file_sink);

        // Create specialized loggers
        auto trade_file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            "logs/trades.log", 1024 * 1024 * 50, 5);
        trade_file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
        trade_logger_ = std::make_shared<spdlog::logger>("trades", trade_file_sink);

        auto order_file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            "logs/orders.log", 1024 * 1024 * 50, 5);
        order_file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
        order_logger_ = std::make_shared<spdlog::logger>("orders", order_file_sink);

        auto market_data_file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            "logs/market_data.log", 1024 * 1024 * 20, 3);
        market_data_file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
        market_data_logger_ = std::make_shared<spdlog::logger>("market_data", market_data_file_sink);

        // Register loggers
        spdlog::register_logger(console_logger_);
        spdlog::register_logger(file_logger_);
        spdlog::register_logger(trade_logger_);
        spdlog::register_logger(order_logger_);
        spdlog::register_logger(market_data_logger_);

        // Set global log level
        spdlog::set_level(to_spdlog_level(file_level));

        // Set flush policy
        spdlog::flush_every(std::chrono::seconds(3));

        initialized_ = true;

        info("Logging system initialized successfully");

    } catch (const std::exception& ex) {
        std::cerr << "Failed to initialize logging: " << ex.what() << std::endl;
        throw;
    }
}

std::shared_ptr<spdlog::logger> Logger::get_console_logger() {
    return console_logger_;
}

std::shared_ptr<spdlog::logger> Logger::get_file_logger() {
    return file_logger_;
}

std::shared_ptr<spdlog::logger> Logger::get_trade_logger() {
    return trade_logger_;
}

std::shared_ptr<spdlog::logger> Logger::get_order_logger() {
    return order_logger_;
}

std::shared_ptr<spdlog::logger> Logger::get_market_data_logger() {
    return market_data_logger_;
}

void Logger::trace(const std::string& message) {
    if (console_logger_) console_logger_->trace(message);
    if (file_logger_) file_logger_->trace(message);
}

void Logger::debug(const std::string& message) {
    if (console_logger_) console_logger_->debug(message);
    if (file_logger_) file_logger_->debug(message);
}

void Logger::info(const std::string& message) {
    if (console_logger_) console_logger_->info(message);
    if (file_logger_) file_logger_->info(message);
}

void Logger::warn(const std::string& message) {
    if (console_logger_) console_logger_->warn(message);
    if (file_logger_) file_logger_->warn(message);
}

void Logger::error(const std::string& message) {
    if (console_logger_) console_logger_->error(message);
    if (file_logger_) file_logger_->error(message);
}

void Logger::critical(const std::string& message) {
    if (console_logger_) console_logger_->critical(message);
    if (file_logger_) file_logger_->critical(message);
}

void Logger::log_order(const std::string& order_id, const std::string& action, const std::string& details) {
    if (order_logger_) {
        order_logger_->info("ORDER_ID={} ACTION={} DETAILS={}", order_id, action, details);
    }
    info("Order " + order_id + " " + action + ": " + details);
}

void Logger::log_trade(const std::string& trade_id, const std::string& order_id, const std::string& details) {
    if (trade_logger_) {
        trade_logger_->info("TRADE_ID={} ORDER_ID={} DETAILS={}", trade_id, order_id, details);
    }
    info("Trade " + trade_id + " (Order " + order_id + "): " + details);
}

void Logger::log_market_data(const std::string& symbol, const std::string& data) {
    if (market_data_logger_) {
        market_data_logger_->debug("SYMBOL={} DATA={}", symbol, data);
    }
}

void Logger::log_risk_event(const std::string& event, const std::string& details) {
    warn("RISK_EVENT: " + event + " - " + details);
}

void Logger::set_console_level(Level level) {
    if (console_logger_) {
        console_logger_->set_level(to_spdlog_level(level));
    }
}

void Logger::set_file_level(Level level) {
    if (file_logger_) {
        file_logger_->set_level(to_spdlog_level(level));
    }
}

void Logger::shutdown() {
    if (initialized_) {
        spdlog::shutdown();
        initialized_ = false;
    }
}

spdlog::level::level_enum Logger::to_spdlog_level(Level level) {
    switch (level) {
        case Level::TRACE: return spdlog::level::trace;
        case Level::DEBUG: return spdlog::level::debug;
        case Level::INFO: return spdlog::level::info;
        case Level::WARN: return spdlog::level::warn;
        case Level::ERROR: return spdlog::level::err;
        case Level::CRITICAL: return spdlog::level::critical;
        default: return spdlog::level::info;
    }
}

void Logger::ensure_directory_exists(const std::string& file_path) {
    std::filesystem::path path(file_path);
    std::filesystem::path directory = path.parent_path();

    if (!directory.empty() && !std::filesystem::exists(directory)) {
        std::filesystem::create_directories(directory);
    }
}

} // namespace trading