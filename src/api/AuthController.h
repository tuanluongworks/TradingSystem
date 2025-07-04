#ifndef AUTHCONTROLLER_H
#define AUTHCONTROLLER_H

#include <string>

class AuthController {
public:
    AuthController();
    bool login(const std::string& username, const std::string& password);
    bool registerUser(const std::string& username, const std::string& password);
    void logout(const std::string& username);
    
    // Token generation
    std::string generateAuthToken(const std::string& userId, const std::string& username);
    bool validateToken(const std::string& token);
};

#endif // AUTHCONTROLLER_H