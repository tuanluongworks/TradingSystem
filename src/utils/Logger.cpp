#include "Logger.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <sstream>
#include <chrono>
#include <mutex>

Logger::Logger(const std::string& filename) {
    logFile.open(filename, std::ios::app);
}

Logger::~Logger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

std::string Logger::isoTime() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
    return std::string(buf);
}

void Logger::log(LogSeverity level, const std::string& message, const std::optional<LogContext>& ctx) {
    if (level < minLevel_) return;
    std::lock_guard<std::mutex> lock(mutex_);
    if (!logFile.is_open()) return;
    auto lvlStr = [level]() {
        switch(level){
            case LogSeverity::DEBUG: return "DEBUG";
            case LogSeverity::INFO: return "INFO";
            case LogSeverity::WARN: return "WARN";
            case LogSeverity::ERROR: return "ERROR";
        }
        return "INFO";
    }();
    logFile << '{' << "\"ts\":\"" << isoTime() << "\",\"level\":\"" << lvlStr << "\",\"msg\":\"";
    for(char c: message){
        if(c=='"') logFile << '\\';
        logFile << c;
    }
    logFile << "\"";
    if (ctx) {
        if (!ctx->correlationId.empty()) logFile << ",\"corr\":\"" << ctx->correlationId << "\"";
        if (!ctx->userId.empty()) logFile << ",\"user\":\"" << ctx->userId << "\"";
    }
    logFile << "}\n";
}