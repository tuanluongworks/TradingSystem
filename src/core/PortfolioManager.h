class PortfolioManager {
public:
    PortfolioManager();
    ~PortfolioManager();

    void addAsset(const std::string& asset, double amount);
    void removeAsset(const std::string& asset);
    double getPortfolioValue() const;

private:
    std::map<std::string, double> assets; // Asset name and its amount
};