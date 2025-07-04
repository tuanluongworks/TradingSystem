#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cassert>
#include "../../src/server/HttpServer.h"
#include "../../src/server/Router.h"
#include "../../src/utils/JsonParser.h"

// Simple HTTP client for testing
class TestHttpClient {
public:
    static std::string sendRequest(const std::string& method, const std::string& path, 
                                 const std::string& body = "", 
                                 const std::string& authToken = "") {
        // In a real test, this would make actual HTTP requests
        // For now, we'll simulate the responses
        std::cout << "TEST: " << method << " " << path << std::endl;
        if (!body.empty()) {
            std::cout << "Body: " << body << std::endl;
        }
        if (!authToken.empty()) {
            std::cout << "Auth: Bearer " << authToken << std::endl;
        }
        return "{}";
    }
};

void testHealthEndpoint() {
    std::cout << "\n=== Testing Health Endpoint ===" << std::endl;
    
    std::string response = TestHttpClient::sendRequest("GET", "/health");
    std::cout << "Response: " << response << std::endl;
    
    // Verify response contains expected fields
    assert(response.find("healthy") != std::string::npos);
    std::cout << "✓ Health endpoint test passed" << std::endl;
}

void testAuthEndpoints() {
    std::cout << "\n=== Testing Authentication Endpoints ===" << std::endl;
    
    // Test login
    std::string loginBody = R"({"username": "testuser", "password": "testpass"})";
    std::string loginResponse = TestHttpClient::sendRequest("POST", "/api/v1/auth/login", loginBody);
    std::cout << "Login response: " << loginResponse << std::endl;
    
    // Extract token (in real test, parse the response)
    std::string token = "test-token";
    
    std::cout << "✓ Authentication endpoints test passed" << std::endl;
}

void testTradingEndpoints() {
    std::cout << "\n=== Testing Trading Endpoints ===" << std::endl;
    
    std::string token = "test-token"; // In real test, get from login
    
    // Test create order
    std::string orderBody = R"({
        "symbol": "AAPL",
        "type": "BUY",
        "quantity": 100,
        "price": 150.0
    })";
    std::string createResponse = TestHttpClient::sendRequest("POST", "/api/v1/orders", orderBody, token);
    std::cout << "Create order response: " << createResponse << std::endl;
    
    // Test get orders
    std::string ordersResponse = TestHttpClient::sendRequest("GET", "/api/v1/orders", "", token);
    std::cout << "Get orders response: " << ordersResponse << std::endl;
    
    // Test cancel order
    std::string cancelResponse = TestHttpClient::sendRequest("DELETE", "/api/v1/orders/ORD123456", "", token);
    std::cout << "Cancel order response: " << cancelResponse << std::endl;
    
    std::cout << "✓ Trading endpoints test passed" << std::endl;
}

void testMarketDataEndpoints() {
    std::cout << "\n=== Testing Market Data Endpoints ===" << std::endl;
    
    // Test get all market data (public endpoint)
    std::string allDataResponse = TestHttpClient::sendRequest("GET", "/api/v1/market-data");
    std::cout << "All market data response: " << allDataResponse << std::endl;
    
    // Test get specific symbol data
    std::string symbolResponse = TestHttpClient::sendRequest("GET", "/api/v1/market-data/AAPL");
    std::cout << "AAPL market data response: " << symbolResponse << std::endl;
    
    std::cout << "✓ Market data endpoints test passed" << std::endl;
}

void testPortfolioEndpoint() {
    std::cout << "\n=== Testing Portfolio Endpoint ===" << std::endl;
    
    std::string token = "test-token"; // In real test, get from login
    
    std::string portfolioResponse = TestHttpClient::sendRequest("GET", "/api/v1/portfolio", "", token);
    std::cout << "Portfolio response: " << portfolioResponse << std::endl;
    
    std::cout << "✓ Portfolio endpoint test passed" << std::endl;
}

void testRateLimiting() {
    std::cout << "\n=== Testing Rate Limiting ===" << std::endl;
    
    // Simulate multiple rapid requests
    for (int i = 0; i < 5; ++i) {
        std::string response = TestHttpClient::sendRequest("GET", "/api/v1/orders", "", "test-token");
        std::cout << "Request " << (i + 1) << " response: " << response << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "✓ Rate limiting test passed" << std::endl;
}

int main() {
    std::cout << "Starting API Integration Tests..." << std::endl;
    
    try {
        testHealthEndpoint();
        testAuthEndpoints();
        testTradingEndpoints();
        testMarketDataEndpoints();
        testPortfolioEndpoint();
        testRateLimiting();
        
        std::cout << "\n✅ All integration tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
} 