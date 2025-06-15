#include "Router.h"
#include "HttpServer.h"
#include <unordered_map>
#include <functional>

class Router {
public:
    Router() = default;

    void addRoute(const std::string& path, const std::function<void(HttpRequest&, HttpResponse&)>& handler) {
        routes[path] = handler;
    }

    void handleRequest(HttpRequest& request, HttpResponse& response) {
        auto it = routes.find(request.getPath());
        if (it != routes.end()) {
            it->second(request, response);
        } else {
            response.setStatus(404);
            response.setBody("Not Found");
        }
    }

private:
    std::unordered_map<std::string, std::function<void(HttpRequest&, HttpResponse&)>> routes;
};