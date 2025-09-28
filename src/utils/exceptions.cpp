#include "exceptions.hpp"

namespace trading {

TradingException::TradingException(const std::string& message)
    : std::runtime_error(message), message_(message) {
}

const char* TradingException::what() const noexcept {
    return message_.c_str();
}

InvalidOrderException::InvalidOrderException(const std::string& message)
    : TradingException("Invalid Order: " + message) {
}

RiskViolationException::RiskViolationException(const std::string& message)
    : TradingException("Risk Violation: " + message) {
}

MarketDataException::MarketDataException(const std::string& message)
    : TradingException("Market Data Error: " + message) {
}

PersistenceException::PersistenceException(const std::string& message)
    : TradingException("Persistence Error: " + message) {
}

ConfigurationException::ConfigurationException(const std::string& message)
    : TradingException("Configuration Error: " + message) {
}

NetworkException::NetworkException(const std::string& message)
    : TradingException("Network Error: " + message) {
}

UIException::UIException(const std::string& message)
    : TradingException("UI Error: " + message) {
}

} // namespace trading