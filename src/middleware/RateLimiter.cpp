#include "RateLimiter.h"
#include "../server/HttpServer.h"
#include <sstream>

std::map<std::string, RateLimiter::ClientInfo> RateLimiter::clientData;
std::mutex RateLimiter::dataMutex;

std::function<void(HttpRequest&, HttpResponse&)> RateLimiter::create(
    int maxRequests, int windowSeconds) {
    
    return [maxRequests, windowSeconds](HttpRequest& request, HttpResponse& response) {
        std::string clientId = getClientIdentifier(request);
        
        if (isRateLimited(clientId, maxRequests, windowSeconds)) {
            response.statusCode = 429;
            response.statusText = "Too Many Requests";
            response.headers["Content-Type"] = "application/json";
            response.headers["Retry-After"] = std::to_string(windowSeconds);
            response.body = R"({"error": "Rate limit exceeded. Please try again later."})";
        }
    };
}

std::function<void(HttpRequest&, HttpResponse&)> RateLimiter::createTokenBucket(const TokenBucketConfig& cfg) {
    return [cfg](HttpRequest& request, HttpResponse& response) {
        std::string clientId = getClientIdentifier(request);
        auto now = std::chrono::steady_clock::now();
        {
            std::lock_guard<std::mutex> lock(dataMutex);
            auto &info = clientData[clientId];
            if (info.lastRefill.time_since_epoch().count() == 0) { info.lastRefill = now; info.tokens = cfg.capacity; }
            double elapsed = std::chrono::duration<double>(now - info.lastRefill).count();
            double refill = elapsed * cfg.refillTokensPerSecond;
            if (refill > 0.0) {
                info.tokens = std::min<double>(cfg.capacity, info.tokens + refill);
                info.lastRefill = now;
            }
            if (info.tokens >= 1.0) {
                info.tokens -= 1.0; // consume
            } else {
                response.statusCode = 429; response.statusText = "Too Many Requests";
                response.headers["Content-Type"] = "application/json";
                response.headers["Retry-After"] = "1"; // approximate
                response.body = R"({"error":"Rate limit (token bucket) exceeded"})";
            }
        }
    };
}

std::string RateLimiter::getClientIdentifier(const HttpRequest& request) {
    // Use IP address from X-Forwarded-For header if available, otherwise use a default
    auto it = request.headers.find("X-Forwarded-For");
    if (it != request.headers.end()) {
        // Get the first IP in the list
        std::string ip = it->second;
        size_t comma = ip.find(',');
        if (comma != std::string::npos) {
            ip = ip.substr(0, comma);
        }
        return ip;
    }
    
    // In a real implementation, you would get the actual client IP
    // For now, use the User-Agent as a simple identifier
    it = request.headers.find("User-Agent");
    if (it != request.headers.end()) {
        return it->second;
    }
    
    return "default-client";
}

bool RateLimiter::isRateLimited(const std::string& clientId, int maxRequests, int windowSeconds) {
    std::lock_guard<std::mutex> lock(dataMutex);
    
    auto now = std::chrono::steady_clock::now();
    auto& info = clientData[clientId];
    
    // Check if we need to reset the window
    auto windowDuration = std::chrono::seconds(windowSeconds);
    if (now - info.windowStart >= windowDuration) {
        info.requestCount = 0;
        info.windowStart = now;
    }
    
    // Increment request count
    info.requestCount++;
    
    // Check if limit exceeded
    return info.requestCount > maxRequests;
}