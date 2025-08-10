#ifndef JWTTOKEN_H
#define JWTTOKEN_H

#include <string>
#include <map>
#include <chrono>

struct JwtValidationParams { 
    std::string expectedIssuer; 
    std::string expectedAudience; 
};

class JwtToken {
public:
    static void setSigningKey(const std::string& key); // external secret injection
    static void setIssuer(const std::string& iss); 
    static void setAudience(const std::string& aud);
    
    // Simple JWT-like token generation (not cryptographically secure - for demo only)
    static std::string generateToken(const std::string& userId, const std::string& username);
    
    // Validate and decode token
    static bool validateToken(const std::string& token, const JwtValidationParams& params = {});
    static std::map<std::string, std::string> decodeToken(const std::string& token);
    
    // Extract user ID from token
    static std::string getUserIdFromToken(const std::string& token);
    
private:
    static std::string base64Encode(const std::string& input);
    static std::string base64Decode(const std::string& input);
    static std::string createSignature(const std::string& data);
    static std::string& secret();
    static std::string& issuer();
    static std::string& audience();
};

#endif // JWTTOKEN_H