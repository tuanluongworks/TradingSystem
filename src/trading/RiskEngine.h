#pragma once
#include "Types.h"
#include <unordered_map>
#include <string>
#include <mutex>
#include <optional>
#include <vector>

struct RiskLimits { double maxPositionPerSymbol = 10000.0; double maxNotionalPerSymbol = 1'000'000.0; double maxPortfolioNotional = 10'000'000.0; };

struct Position { double quantity{0}; double avgPrice{0}; };

class RiskEngine {
public:
    explicit RiskEngine(RiskLimits limits = {});
    bool preValidateNewOrder(const Order& order, std::string& reason) const; // fast checks
    void onOrderExecuted(const Order& order);
    double currentSymbolExposure(const std::string& symbol) const;
    double currentPortfolioNotional() const;
private:
    RiskLimits limits_;
    mutable std::mutex mtx_;
    std::unordered_map<std::string, Position> positions_; // symbol -> position
};
