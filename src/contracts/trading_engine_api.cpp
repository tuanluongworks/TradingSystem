#include "contracts/trading_engine_api.hpp"
#include <cmath>

namespace trading {

bool OrderRequest::is_valid() const {
    // Check instrument symbol
    if (instrument_symbol.empty()) {
        return false;
    }

    // Check quantity is positive
    if (quantity <= 0.0) {
        return false;
    }

    // Check price for limit orders
    if (type == OrderType::LIMIT && price <= 0.0) {
        return false;
    }

    // Market orders should have price = 0
    if (type == OrderType::MARKET && price != 0.0) {
        return false;
    }

    // Check for reasonable timestamp (not in future by more than 1 minute)
    auto now = std::chrono::system_clock::now();
    auto max_future = now + std::chrono::minutes(1);
    if (timestamp > max_future) {
        return false;
    }

    // Check for reasonable timestamp (not more than 1 day old)
    auto min_past = now - std::chrono::hours(24);
    if (timestamp < min_past) {
        return false;
    }

    return true;
}

} // namespace trading