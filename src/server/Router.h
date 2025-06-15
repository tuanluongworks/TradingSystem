#ifndef ROUTER_H
#define ROUTER_H

#include <string>
#include <functional>
#include <map>

class Router {
public:
    using Handler = std::function<void()>;

    void addRoute(const std::string& path, Handler handler);
    void handleRequest(const std::string& path);

private:
    std::map<std::string, Handler> routes;
};

#endif // ROUTER_H