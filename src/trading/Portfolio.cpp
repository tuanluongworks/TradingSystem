#include "Portfolio.h"

Portfolio::Portfolio() {
    // Constructor implementation
}

void Portfolio::addAsset(const Asset& asset) {
    // Add asset to the portfolio
    assets.push_back(asset);
}

void Portfolio::removeAsset(const std::string& assetId) {
    // Remove asset from the portfolio by asset ID
    assets.erase(std::remove_if(assets.begin(), assets.end(),
        [&assetId](const Asset& asset) { return asset.getId() == assetId; }), assets.end());
}

std::vector<Asset> Portfolio::getAssets() const {
    // Return the list of assets in the portfolio
    return assets;
}