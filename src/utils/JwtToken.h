#ifndef JWTTOKEN_H
#define JWTTOKEN_H

#include <string>
#include <map>
#include <chrono>

class JwtToken {
public:
    // Simple JWT-like token generation (not cryptographically secure - for demo only)
    static std::string generateToken(const std::string& userId, const std::string& username);
    
    // Validate and decode token
    static bool validateToken(const std::string& token);
    static std::map<std::string, std::string> decodeToken(const std::string& token);
    
    // Extract user ID from token
    static std::string getUserIdFromToken(const std::string& token);
    
private:
    static std::string base64Encode(const std::string& input);
    static std::string base64Decode(const std::string& input);
    static std::string createSignature(const std::string& data);
};

#endif // JWTTOKEN_H 