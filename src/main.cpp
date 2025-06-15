#include <iostream>
#include "server/HttpServer.h"

int main() {
    HttpServer server;

    // Start the server
    server.start();

    std::cout << "Server is running. Press Enter to stop..." << std::endl;
    std::cin.get();

    // Stop the server
    server.stop();

    return 0;
}