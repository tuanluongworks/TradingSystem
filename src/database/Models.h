#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <vector>

class User {
public:
    User(int id, const std::string& name, const std::string& email);
    
    int getId() const;
    std::string getName() const;
    std::string getEmail() const;

private:
    int id;
    std::string name;
    std::string email;
};

class Asset {
public:
    Asset(int id, const std::string& symbol, double quantity);
    
    int getId() const;
    std::string getSymbol() const;
    double getQuantity() const;

private:
    int id;
    std::string symbol;
    double quantity;
};

class Order {
public:
    Order(int id, int userId, const std::string& assetSymbol, double quantity, const std::string& orderType);
    
    int getId() const;
    int getUserId() const;
    std::string getAssetSymbol() const;
    double getQuantity() const;
    std::string getOrderType() const;

private:
    int id;
    int userId;
    std::string assetSymbol;
    double quantity;
    std::string orderType;
};

#endif // MODELS_H