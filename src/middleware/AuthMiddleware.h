#ifndef AUTHMIDDLEWARE_H
#define AUTHMIDDLEWARE_H

#include <string>
#include <functional>

struct HttpRequest;
struct HttpResponse;

class AuthMiddleware {
public:
    // Middleware function to check authentication
    static std::function<void(HttpRequest&, HttpResponse&)> checkAuth();
    
    // Extract token from Authorization header
    static std::string extractToken(const HttpRequest& request);
    
    // Check if a path requires authentication
    static bool requiresAuth(const std::string& path);
};

#endif // AUTHMIDDLEWARE_H 