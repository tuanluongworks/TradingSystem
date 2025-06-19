#ifndef ROUTER_H
#define ROUTER_H

#include <string>
#include <functional>
#include <map>
#include <memory>
#include <regex>

// Forward declarations
struct HttpRequest;
struct HttpResponse;

class Router {
public:
    using Handler = std::function<HttpResponse(const HttpRequest&)>;
    
    Router();
    ~Router() = default;
    
    // Route registration
    void get(const std::string& path, Handler handler);
    void post(const std::string& path, Handler handler);
    void put(const std::string& path, Handler handler);
    void del(const std::string& path, Handler handler);
    
    // Route handling
    HttpResponse route(const HttpRequest& request);
    
    // Middleware support
    void use(std::function<void(HttpRequest&, HttpResponse&)> middleware);

private:
    struct Route {
        std::string method;
        std::regex pattern;
        Handler handler;
    };
    
    std::vector<Route> routes;
    std::vector<std::function<void(HttpRequest&, HttpResponse&)>> middlewares;
    
    void addRoute(const std::string& method, const std::string& path, Handler handler);
    HttpResponse handleNotFound();
    HttpResponse handleMethodNotAllowed();
};

#endif // ROUTER_H