#include "Router.h"
#include "HttpServer.h"
#include <algorithm>
#include <sstream>
#include <regex>
#include <vector>

Router::Router() {}

void Router::get(const std::string& path, Handler handler) {
    addRoute("GET", path, handler);
}

void Router::post(const std::string& path, Handler handler) {
    addRoute("POST", path, handler);
}

void Router::put(const std::string& path, Handler handler) {
    addRoute("PUT", path, handler);
}

void Router::del(const std::string& path, Handler handler) {
    addRoute("DELETE", path, handler);
}

void Router::addRoute(const std::string& method, const std::string& path, Handler handler) {
    // Convert path to regex pattern
    std::string pattern = path;
    
    // Escape special regex characters except for parameter placeholders
    pattern = std::regex_replace(pattern, std::regex(R"([\^\$\.\|\?\*\+\(\)\[\]\{\}])"), R"(\$&)");
    
    // Convert :param to capturing groups (not named groups for compatibility)
    pattern = std::regex_replace(pattern, std::regex(R"(:(\w+))"), R"(([^/]+))");
    
    // Add start and end anchors
    pattern = "^" + pattern + "$";
    
    routes.push_back({method, std::regex(pattern), handler});
}

HttpResponse Router::route(const HttpRequest& request) {
    HttpResponse response;
    
    // Apply middlewares
    for (auto& middleware : middlewares) {
        middleware(const_cast<HttpRequest&>(request), response);
    }
    
    // Find matching route
    for (const auto& route : routes) {
        if (route.method == request.method) {
            std::smatch match;
            if (std::regex_match(request.path, match, route.pattern)) {
                try {
                    return route.handler(request);
                } catch (const std::exception& e) {
                    response.statusCode = 500;
                    response.statusText = "Internal Server Error";
                    response.body = "Error: " + std::string(e.what());
                    return response;
                }
            }
        }
    }
    
    // Check if path exists with different method
    bool pathExists = false;
    for (const auto& route : routes) {
        std::smatch match;
        if (std::regex_match(request.path, match, route.pattern)) {
            pathExists = true;
            break;
        }
    }
    
    if (pathExists) {
        return handleMethodNotAllowed();
    } else {
        return handleNotFound();
    }
}

void Router::use(std::function<void(HttpRequest&, HttpResponse&)> middleware) {
    middlewares.push_back(middleware);
}

HttpResponse Router::handleNotFound() {
    HttpResponse response;
    response.statusCode = 404;
    response.statusText = "Not Found";
    response.headers["Content-Type"] = "application/json";
    response.body = R"({"error": "Resource not found"})";
    return response;
}

HttpResponse Router::handleMethodNotAllowed() {
    HttpResponse response;
    response.statusCode = 405;
    response.statusText = "Method Not Allowed";
    response.headers["Content-Type"] = "application/json";
    response.body = R"({"error": "Method not allowed for this resource"})";
    return response;
}