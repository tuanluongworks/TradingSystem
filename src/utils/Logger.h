#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <optional>

#ifdef ERROR
#undef ERROR
#endif

enum class LogSeverity { DEBUG=0, INFO=1, WARN=2, ERROR=3 };

struct LogContext { std::string correlationId; std::string userId; };

class Logger {
public:
    Logger(const std::string& filename);
    ~Logger();
    void log(LogSeverity level, const std::string& message, const std::optional<LogContext>& ctx = std::nullopt);
    void setMinimumLevel(LogSeverity lvl) { minLevel_ = lvl; }
private:
    std::ofstream logFile; std::mutex mutex_; LogSeverity minLevel_{LogSeverity::INFO};
    std::string isoTime();
};
#endif // LOGGER_H