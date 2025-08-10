#include "RiskEngine.h"
#include <algorithm>

RiskEngine::RiskEngine(RiskLimits limits) : limits_(limits) {}

bool RiskEngine::preValidateNewOrder(const Order& order, std::string& reason) const {
    std::scoped_lock lock(mtx_);
    double notional = order.quantity * order.price;
    if (notional > limits_.maxNotionalPerSymbol) { reason = "Order notional exceeds per-symbol limit"; return false; }
    auto it = positions_.find(order.symbol);
    double projectedQty = (it!=positions_.end()? it->second.quantity : 0.0) + (order.type==OrderType::BUY? order.quantity : -order.quantity);
    if (std::abs(projectedQty) > limits_.maxPositionPerSymbol) { reason = "Projected position exceeds symbol limit"; return false; }
    double portfolioNotional = 0.0; for (auto &p: positions_) portfolioNotional += std::abs(p.second.quantity * p.second.avgPrice); portfolioNotional += notional; if (portfolioNotional > limits_.maxPortfolioNotional) { reason = "Portfolio notional limit exceeded"; return false; }
    return true;
}

void RiskEngine::onOrderExecuted(const Order& order) {
    std::scoped_lock lock(mtx_);
    auto &pos = positions_[order.symbol];
    double signedQty = (order.type==OrderType::BUY? order.quantity : -order.quantity);
    double newQty = pos.quantity + signedQty;
    if (newQty == 0) { pos.quantity = 0; pos.avgPrice = 0; return; }
    if ((pos.quantity == 0) || ( (pos.quantity>0) == (signedQty>0) )) { // adding to existing side
        double totalCost = pos.quantity * pos.avgPrice + signedQty * order.price;
        pos.quantity = newQty; pos.avgPrice = totalCost / newQty;
    } else { // reducing / flipping
        pos.quantity = newQty;
        if ((pos.quantity>0) != (signedQty>0)) { pos.avgPrice = order.price; } // flipped side baseline
    }
}

double RiskEngine::currentSymbolExposure(const std::string& symbol) const {
    std::scoped_lock lock(mtx_);
    auto it = positions_.find(symbol); if (it==positions_.end()) return 0.0; return it->second.quantity * it->second.avgPrice;
}

double RiskEngine::currentPortfolioNotional() const {
    std::scoped_lock lock(mtx_);
    double total=0; for (auto &p: positions_) total += std::abs(p.second.quantity * p.second.avgPrice); return total;
}
