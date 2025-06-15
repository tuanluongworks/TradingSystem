#ifndef AUTHCONTROLLER_H
#define AUTHCONTROLLER_H

#include <string>

class AuthController {
public:
    AuthController();
    bool login(const std::string& username, const std::string& password);
    bool registerUser(const std::string& username, const std::string& password);
    void logout(const std::string& username);
};

#endif // AUTHCONTROLLER_H