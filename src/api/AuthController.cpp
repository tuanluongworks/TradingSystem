#include "AuthController.h"
#include "../utils/JwtToken.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <random>

AuthController::AuthController() {
    // Initialize with some test users
    // In production, this would connect to a database
}

bool AuthController::login(const std::string& username, const std::string& password) {
    std::cout << "Attempting login for user: " << username << std::endl;
    
    // TODO: In production, verify against database with hashed passwords
    // For now, accept a test user
    if (username == "testuser" && password == "testpass") {
        std::cout << "Login successful for user: " << username << std::endl;
        return true;
    }
    
    std::cout << "Login failed for user: " << username << std::endl;
    return false;
}

bool AuthController::registerUser(const std::string& username, const std::string& password) {
    std::cout << "Registering new user: " << username << std::endl;
    
    // TODO: In production, check if user exists and store in database
    // For now, simulate successful registration
    if (!username.empty() && password.length() >= 6) {
        std::cout << "User registered successfully: " << username << std::endl;
        return true;
    }
    
    std::cout << "Registration failed for user: " << username << std::endl;
    return false;
}

void AuthController::logout(const std::string& username) {
    std::cout << "User logged out: " << username << std::endl;
    // TODO: In production, invalidate session/token
}

std::string AuthController::generateAuthToken(const std::string& userId, const std::string& username) {
    return JwtToken::generateToken(userId, username);
}

bool AuthController::validateToken(const std::string& token) {
    return JwtToken::validateToken(token);
}