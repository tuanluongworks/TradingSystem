#include <gtest/gtest.h>
#include "HttpServer.h"

class HttpServerTest : public ::testing::Test {
protected:
    HttpServer* server;

    void SetUp() override {
        server = new HttpServer();
    }

    void TearDown() override {
        delete server;
    }
};

TEST_F(HttpServerTest, StartServer) {
    EXPECT_NO_THROW(server->start());
}

TEST_F(HttpServerTest, StopServer) {
    server->start();
    EXPECT_NO_THROW(server->stop());
}

TEST_F(HttpServerTest, HandleRequest) {
    server->start();
    HttpRequest request("GET", "/api/test");
    HttpResponse response = server->handleRequest(request);
    EXPECT_EQ(response.getStatusCode(), 200);
    server->stop();
}