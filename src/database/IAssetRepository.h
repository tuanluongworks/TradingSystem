#pragma once
#include "../trading/Types.h"
#include <vector>
#include <string>

class IAssetRepository {
public:
    virtual ~IAssetRepository() = default;
    virtual bool save(const std::string& userId, const Asset& asset) = 0;
    virtual std::vector<Asset> findAssetsByUserId(const std::string& userId) = 0;
    virtual bool update(const std::string& userId, const Asset& asset) = 0;
};
