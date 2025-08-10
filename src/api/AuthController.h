#ifndef AUTHCONTROLLER_H
#define AUTHCONTROLLER_H

#include <string>
#include "../interfaces/IAuthService.h"

class AuthController : public IAuthService {
public:
    AuthController();
    bool login(const std::string& username, const std::string& password) override;
    bool registerUser(const std::string& username, const std::string& password) override;
    void logout(const std::string& username) override;
    std::string generateAuthToken(const std::string& userId, const std::string& username) override;
    bool validateToken(const std::string& token) override;
};

#endif // AUTHCONTROLLER_H