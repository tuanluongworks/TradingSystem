#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <string>
#include <memory>

class HttpServer {
public:
    HttpServer(int port);
    void start();
    void stop();

private:
    int port;
    // Additional private members for handling requests and responses
};

#endif // HTTPSERVER_H