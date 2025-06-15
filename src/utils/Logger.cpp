#include "Logger.h"
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>

Logger::Logger(const std::string& filename) : logFile(filename, std::ios::app) {
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file: " << filename << std::endl;
    }
}

Logger::~Logger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

void Logger::logInfo(const std::string& message) {
    if (logFile.is_open()) {
        logFile << getCurrentTime() << " [INFO]: " << message << std::endl;
    }
}

void Logger::logError(const std::string& message) {
    if (logFile.is_open()) {
        logFile << getCurrentTime() << " [ERROR]: " << message << std::endl;
    }
}

std::string Logger::getCurrentTime() {
    std::time_t now = std::time(nullptr);
    char buf[100];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    return buf;
}