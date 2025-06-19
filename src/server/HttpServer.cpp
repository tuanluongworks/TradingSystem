#include "HttpServer.h"
#include "Router.h"
#include "../utils/Logger.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <chrono>

#ifndef _WIN32
    #include <fcntl.h>
#endif

std::string HttpResponse::toString() const {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
    
    for (const auto& header : headers) {
        oss << header.first << ": " << header.second << "\r\n";
    }
    
    if (!body.empty()) {
        oss << "Content-Length: " << body.length() << "\r\n";
    }
    
    oss << "\r\n";
    if (!body.empty()) {
        oss << body;
    }
    
    return oss.str();
}

HttpServer::HttpServer(int port) : port(port), serverSocket(INVALID_SOCKET), running(false) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
#endif
}

HttpServer::~HttpServer() {
    stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

void HttpServer::start() {
    if (running) {
        return;
    }
    
    if (!initializeSocket()) {
        throw std::runtime_error("Failed to initialize socket");
    }
    
    running = true;
    serverThread = std::thread(&HttpServer::serverLoop, this);
    
    std::cout << "HTTP Server started on port " << port << std::endl;
}

void HttpServer::stop() {
    if (!running) {
        return;
    }
    
    running = false;
    cleanupSocket();
    
    if (serverThread.joinable()) {
        serverThread.join();
    }
    
    std::cout << "HTTP Server stopped" << std::endl;
}

bool HttpServer::initializeSocket() {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        return false;
    }
    
    // Allow socket reuse
    int opt = 1;
#ifdef _WIN32
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
#else
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
#endif
        closesocket(serverSocket);
        return false;
    }
    
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);
    
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(serverSocket);
        return false;
    }
    
    if (listen(serverSocket, 10) == SOCKET_ERROR) {
        closesocket(serverSocket);
        return false;
    }
    
    return true;
}

void HttpServer::cleanupSocket() {
    if (serverSocket != INVALID_SOCKET) {
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
    }
}

void HttpServer::serverLoop() {
    while (running) {
        sockaddr_in clientAddr{};
        socklen_t clientAddrLen = sizeof(clientAddr);
        
        // Set socket to non-blocking mode for accept with timeout
#ifdef _WIN32
        u_long mode = 1;
        ioctlsocket(serverSocket, FIONBIO, &mode);
#else
        int flags = fcntl(serverSocket, F_GETFL, 0);
        fcntl(serverSocket, F_SETFL, flags | O_NONBLOCK);
#endif
        
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrLen);
        
        if (clientSocket != INVALID_SOCKET) {
            // Set client socket back to blocking mode
#ifdef _WIN32
            mode = 0;
            ioctlsocket(clientSocket, FIONBIO, &mode);
#else
            flags = fcntl(clientSocket, F_GETFL, 0);
            fcntl(clientSocket, F_SETFL, flags & ~O_NONBLOCK);
#endif
            
            std::thread clientThread(&HttpServer::handleClient, this, clientSocket);
            clientThread.detach();
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void HttpServer::handleClient(SOCKET clientSocket) {
    const int bufferSize = 4096;
    char buffer[bufferSize];
    std::string requestData;
    
    // Read request
    int bytesReceived;
    do {
        bytesReceived = recv(clientSocket, buffer, bufferSize - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            requestData += buffer;
            
            // Check if we've received the complete headers
            if (requestData.find("\r\n\r\n") != std::string::npos) {
                break;
            }
        }
    } while (bytesReceived > 0);
    
    if (!requestData.empty()) {
        try {
            HttpRequest request = parseRequest(requestData);
            std::string response = handleRequest(request);
            send(clientSocket, response.c_str(), response.length(), 0);
        } catch (const std::exception& e) {
            HttpResponse errorResponse;
            errorResponse.statusCode = 400;
            errorResponse.statusText = "Bad Request";
            errorResponse.body = "Error: " + std::string(e.what());
            std::string response = errorResponse.toString();
            send(clientSocket, response.c_str(), response.length(), 0);
        }
    }
    
    closesocket(clientSocket);
}

HttpRequest HttpServer::parseRequest(const std::string& rawRequest) {
    HttpRequest request;
    std::istringstream stream(rawRequest);
    std::string line;
    
    // Parse request line
    if (std::getline(stream, line)) {
        // Remove \r if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        std::istringstream lineStream(line);
        lineStream >> request.method >> request.path >> request.version;
    }
    
    // Parse headers
    while (std::getline(stream, line) && line != "\r" && !line.empty()) {
        if (line.back() == '\r') {
            line.pop_back();
        }
        
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            
            // Trim whitespace
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            request.headers[key] = value;
        }
    }
    
    // Parse body if present
    std::string remaining((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    request.body = remaining;
    
    return request;
}

std::string HttpServer::handleRequest(const HttpRequest& request) {
    HttpResponse response;
    
    if (router) {
        response = router->route(request);
    } else {
        response.statusCode = 404;
        response.statusText = "Not Found";
        response.body = "No router configured";
    }
    
    return response.toString();
}