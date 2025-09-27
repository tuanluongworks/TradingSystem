#pragma once

#include <stdexcept>
#include <string>

namespace trading {

/**
 * Base exception class for all trading system exceptions
 */
class TradingException : public std::runtime_error {
public:
    explicit TradingException(const std::string& message);
    const char* what() const noexcept override;

protected:
    std::string message_;
};

/**
 * Exception for invalid orders
 */
class InvalidOrderException : public TradingException {
public:
    explicit InvalidOrderException(const std::string& message);
};

/**
 * Exception for risk management violations
 */
class RiskViolationException : public TradingException {
public:
    explicit RiskViolationException(const std::string& message);
};

/**
 * Exception for market data issues
 */
class MarketDataException : public TradingException {
public:
    explicit MarketDataException(const std::string& message);
};

/**
 * Exception for persistence/database issues
 */
class PersistenceException : public TradingException {
public:
    explicit PersistenceException(const std::string& message);
};

/**
 * Exception for configuration issues
 */
class ConfigurationException : public TradingException {
public:
    explicit ConfigurationException(const std::string& message);
};

/**
 * Exception for network/connection issues
 */
class NetworkException : public TradingException {
public:
    explicit NetworkException(const std::string& message);
};

/**
 * Exception for UI-related issues
 */
class UIException : public TradingException {
public:
    explicit UIException(const std::string& message);
};

} // namespace trading