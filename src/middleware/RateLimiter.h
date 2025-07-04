#ifndef RATELIMITER_H
#define RATELIMITER_H

#include <string>
#include <map>
#include <chrono>
#include <mutex>
#include <functional>

struct HttpRequest;
struct HttpResponse;

class RateLimiter {
public:
    // Create rate limiter middleware with specified limits
    static std::function<void(HttpRequest&, HttpResponse&)> create(
        int maxRequests = 100,  // Max requests per window
        int windowSeconds = 60  // Time window in seconds
    );
    
private:
    struct ClientInfo {
        int requestCount;
        std::chrono::steady_clock::time_point windowStart;
    };
    
    static std::map<std::string, ClientInfo> clientData;
    static std::mutex dataMutex;
    
    static std::string getClientIdentifier(const HttpRequest& request);
    static bool isRateLimited(const std::string& clientId, int maxRequests, int windowSeconds);
};

#endif // RATELIMITER_H 