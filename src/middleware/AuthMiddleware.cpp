#include "AuthMiddleware.h"
#include "../server/HttpServer.h"
#include "../utils/JwtToken.h"
#include <algorithm>
#include <vector>

std::function<void(HttpRequest&, HttpResponse&)> AuthMiddleware::checkAuth() {
    return [](HttpRequest& request, HttpResponse& response) {
        // Check if the path requires authentication
        if (!requiresAuth(request.path)) {
            return; // No auth required for this path
        }
        
        // Extract token from Authorization header
        std::string token = extractToken(request);
        
        if (token.empty()) {
            response.statusCode = 401;
            response.statusText = "Unauthorized";
            response.headers["Content-Type"] = "application/json";
            response.body = R"({"error": "No authentication token provided"})";
            return;
        }
        
        // Validate token
        if (!JwtToken::validateToken(token)) {
            response.statusCode = 401;
            response.statusText = "Unauthorized";
            response.headers["Content-Type"] = "application/json";
            response.body = R"({"error": "Invalid or expired token"})";
            return;
        }
        
        // Extract user information from token and add to request
        try {
            std::string userId = JwtToken::getUserIdFromToken(token);
            request.headers["X-User-Id"] = userId;
        } catch (...) {
            response.statusCode = 401;
            response.statusText = "Unauthorized";
            response.headers["Content-Type"] = "application/json";
            response.body = R"({"error": "Invalid token format"})";
        }
    };
}

std::string AuthMiddleware::extractToken(const HttpRequest& request) {
    auto it = request.headers.find("Authorization");
    if (it != request.headers.end()) {
        const std::string& authHeader = it->second;
        const std::string bearer = "Bearer ";
        
        if (authHeader.size() > bearer.size() && 
            authHeader.substr(0, bearer.size()) == bearer) {
            return authHeader.substr(bearer.size());
        }
    }
    
    return "";
}

bool AuthMiddleware::requiresAuth(const std::string& path) {
    // Define paths that don't require authentication
    static const std::vector<std::string> publicPaths = {
        "/health",
        "/api/v1/auth/login",
        "/api/v1/auth/register",
        "/api/v1/market-data"  // Public market data
    };
    
    // Check if path is in public paths
    for (const auto& publicPath : publicPaths) {
        if (path == publicPath || path.find(publicPath) == 0) {
            return false;
        }
    }
    
    // All other paths require authentication
    return path.find("/api/v1/") == 0;
} 