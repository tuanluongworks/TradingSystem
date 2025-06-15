#ifndef PORTFOLIO_H
#define PORTFOLIO_H

#include <vector>
#include <string>

class Portfolio {
public:
    Portfolio();
    ~Portfolio();

    void addAsset(const std::string& asset);
    void removeAsset(const std::string& asset);
    const std::vector<std::string>& getAssets() const;

private:
    std::vector<std::string> assets;
};

#endif // PORTFOLIO_H