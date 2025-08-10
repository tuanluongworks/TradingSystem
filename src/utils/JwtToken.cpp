#include "JwtToken.h"
#include "JsonParser.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <functional>

static std::string g_jwt_secret = "development-secret-change"; // overridden via setSigningKey
static std::string g_jwt_issuer = "trading-system";
static std::string g_jwt_audience = "trading-clients";

void JwtToken::setSigningKey(const std::string& key) { g_jwt_secret = key; }
void JwtToken::setIssuer(const std::string& iss) { g_jwt_issuer = iss; }
void JwtToken::setAudience(const std::string& aud) { g_jwt_audience = aud; }
std::string& JwtToken::secret() { return g_jwt_secret; }
std::string& JwtToken::issuer() { return g_jwt_issuer; }
std::string& JwtToken::audience() { return g_jwt_audience; }

std::string JwtToken::generateToken(const std::string& userId, const std::string& username) {
    // Create header
    std::map<std::string, std::string> header;
    header["alg"] = "HS256";
    header["typ"] = "JWT";
    std::string headerJson = JsonParser::createObject(header);
    
    // Create payload with expiration time (1 hour from now)
    auto now = std::chrono::system_clock::now();
    auto exp = now + std::chrono::hours(1);
    auto exp_time = std::chrono::duration_cast<std::chrono::seconds>(exp.time_since_epoch()).count();
    
    std::map<std::string, std::string> payload;
    payload["userId"] = userId;
    payload["username"] = username;
    payload["exp"] = std::to_string(exp_time);
    payload["iat"] = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());
    payload["iss"] = issuer();
    payload["aud"] = audience();
    std::string encodedHeader = base64Encode(headerJson);
    std::string encodedPayload = base64Encode(JsonParser::createObject(payload));
    
    // Create signature
    std::string dataToSign = encodedHeader + "." + encodedPayload;
    std::string signature = createSignature(dataToSign);
    
    // Combine all parts
    return encodedHeader + "." + encodedPayload + "." + signature;
}

bool JwtToken::validateToken(const std::string& token, const JwtValidationParams& params) {
    // Split token into parts
    size_t firstDot = token.find('.');
    size_t secondDot = token.find('.', firstDot + 1);
    
    if (firstDot == std::string::npos || secondDot == std::string::npos) {
        return false;
    }
    
    std::string encodedHeader = token.substr(0, firstDot);
    std::string encodedPayload = token.substr(firstDot + 1, secondDot - firstDot - 1);
    std::string providedSig = token.substr(secondDot + 1);
    
    // Verify signature
    std::string dataToSign = encodedHeader + "." + encodedPayload;
    std::string expected = createSignature(dataToSign);
    if (providedSig != expected) {
        return false;
    }
    try {
        std::string headerJson = base64Decode(encodedHeader);
        std::string alg = JsonParser::extractString(headerJson, "alg");
        if (alg != "HS256") return false;
        
        std::string payloadJson = base64Decode(encodedPayload);
        double exp = JsonParser::extractNumber(payloadJson, "exp");
        auto now = std::chrono::system_clock::now();
        auto now_time = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        if (exp <= now_time) return false;
        
        std::string iss = JsonParser::extractString(payloadJson, "iss");
        if (!params.expectedIssuer.empty() && iss != params.expectedIssuer) return false;
        if (params.expectedIssuer.empty() && iss != issuer()) return false;
        
        std::string aud = JsonParser::extractString(payloadJson, "aud");
        if (!params.expectedAudience.empty() && aud != params.expectedAudience) return false;
        if (params.expectedAudience.empty() && aud != audience()) return false;
        
        return true;
    } catch (...) {
        return false;
    }
}

std::map<std::string, std::string> JwtToken::decodeToken(const std::string& token) {
    std::map<std::string, std::string> result;
    
    size_t firstDot = token.find('.');
    size_t secondDot = token.find('.', firstDot + 1);
    
    if (firstDot != std::string::npos && secondDot != std::string::npos) {
        std::string encodedPayload = token.substr(firstDot + 1, secondDot - firstDot - 1);
        std::string payloadJson = base64Decode(encodedPayload);
        
        result["userId"] = JsonParser::extractString(payloadJson, "userId");
        result["username"] = JsonParser::extractString(payloadJson, "username");
        result["exp"] = JsonParser::extractString(payloadJson, "exp");
        result["iat"] = JsonParser::extractString(payloadJson, "iat");
        result["iss"] = JsonParser::extractString(payloadJson, "iss");
        result["aud"] = JsonParser::extractString(payloadJson, "aud");
    }
    
    return result;
}

std::string JwtToken::getUserIdFromToken(const std::string& token) {
    auto decoded = decodeToken(token);
    return decoded["userId"];
}

std::string JwtToken::base64Encode(const std::string& input) {
    static const std::string base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::string encoded;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    
    for (char c : input) {
        char_array_3[i++] = c;
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            
            for(i = 0; i < 4; i++)
                encoded += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    
    if (i) {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';
        
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        
        for (j = 0; j < i + 1; j++)
            encoded += base64_chars[char_array_4[j]];
        
        while(i++ < 3)
            encoded += '=';
    }
    
    // URL-safe base64 (replace + with -, / with _)
    std::replace(encoded.begin(), encoded.end(), '+', '-');
    std::replace(encoded.begin(), encoded.end(), '/', '_');
    // Remove padding for JWT
    encoded.erase(std::remove(encoded.begin(), encoded.end(), '='), encoded.end());
    
    return encoded;
}

std::string JwtToken::base64Decode(const std::string& input) {
    static const std::string base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::string encoded = input;
    // Convert URL-safe base64 back to standard
    std::replace(encoded.begin(), encoded.end(), '-', '+');
    std::replace(encoded.begin(), encoded.end(), '_', '/');
    
    // Add padding if necessary
    while (encoded.length() % 4 != 0) {
        encoded += '=';
    }
    
    std::string decoded;
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    
    while (in_ < encoded.size() && encoded[in_] != '=') {
        char_array_4[i++] = encoded[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);
            
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            
            for (i = 0; i < 3; i++)
                decoded += char_array_3[i];
            i = 0;
        }
    }
    
    if (i) {
        for (j = 0; j < i; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);
        
        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        
        for (j = 0; j < i - 1; j++)
            decoded += char_array_3[j];
    }
    
    return decoded;
}

std::string JwtToken::createSignature(const std::string& data) {
    std::hash<std::string> hasher;
    size_t hash = hasher(data + secret());
    
    std::ostringstream oss;
    oss << std::hex << hash;
    return base64Encode(oss.str());
}