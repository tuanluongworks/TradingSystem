#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>

class Logger {
public:
    enum LogLevel {
        INFO,
        WARNING,
        LOG_ERROR  // Renamed to avoid Windows macro conflict
    };

    Logger(const std::string& filename);
    ~Logger();

    void log(const std::string& message, LogLevel level = INFO);

private:
    std::ofstream logFile;

    std::string getCurrentTime();
    std::string logLevelToString(LogLevel level);
};

#endif // LOGGER_H