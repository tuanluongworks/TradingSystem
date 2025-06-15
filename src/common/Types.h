#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <vector>
#include <unordered_map>

namespace TradingSystem {

    // Define common types used throughout the application

    using OrderID = std::string;
    using UserID = std::string;
    using AssetID = std::string;

    struct Order {
        OrderID id;
        UserID userId;
        AssetID assetId;
        double quantity;
        double price;
        std::string side; // "buy" or "sell"
    };

    struct User {
        UserID id;
        std::string username;
        std::string passwordHash; // Store hashed password
        std::vector<AssetID> portfolio; // List of assets owned by the user
    };

    struct MarketData {
        AssetID assetId;
        double price;
        double volume;
    };

} // namespace TradingSystem

#endif // TYPES_H