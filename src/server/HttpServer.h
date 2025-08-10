#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <map>
#include "../infrastructure/ThreadPool.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
    typedef int SOCKET;
#endif

class Router;

struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
    std::map<std::string, std::string> pathParams;
    std::string correlationId;
    std::string userId;
};

struct HttpResponse {
    int statusCode;
    std::string statusText;
    std::map<std::string, std::string> headers;
    std::string body;
    
    HttpResponse() : statusCode(200), statusText("OK") {
        headers["Content-Type"] = "text/plain";
        headers["Server"] = "TradingSystem/1.0";
    }
    
    std::string toString() const;
};

class HttpServer {
public:
    HttpServer(int port = 8080);
    ~HttpServer();
    
    void start();
    void stop();
    bool isRunning() const { return running; }
    
    void setRouter(std::shared_ptr<Router> router) { this->router = router; }

private:
    int port;
    SOCKET serverSocket;
    std::atomic<bool> running;
    std::thread serverThread;
    std::shared_ptr<Router> router;
    ThreadPool requestPool_{4}; // initial fixed worker size; could be made configurable
    
    void serverLoop();
    void handleClient(SOCKET clientSocket);
    HttpRequest parseRequest(const std::string& rawRequest);
    std::string handleRequest(const HttpRequest& request);
    
    bool initializeSocket();
    void cleanupSocket();
};

#endif // HTTPSERVER_H