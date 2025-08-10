#pragma once
#include <string>

// Authentication boundary. Hides token generation / validation specifics.
class IAuthService {
public:
    virtual ~IAuthService() = default;
    virtual bool login(const std::string& username, const std::string& password) = 0;
    virtual bool registerUser(const std::string& username, const std::string& password) = 0;
    virtual void logout(const std::string& username) = 0;
    virtual std::string generateAuthToken(const std::string& userId, const std::string& username) = 0;
    virtual bool validateToken(const std::string& token) = 0;
};
