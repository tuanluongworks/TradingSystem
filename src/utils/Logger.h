class Logger {
public:
    // Logs an informational message
    void logInfo(const std::string& message);

    // Logs an error message
    void logError(const std::string& message);

    // Logs a warning message
    void logWarning(const std::string& message);
};