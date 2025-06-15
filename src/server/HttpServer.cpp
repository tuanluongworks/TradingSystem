#include "HttpServer.h"
#include "Router.h"
#include <iostream>
#include <string>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

HttpServer::HttpServer(unsigned short port)
    : acceptor_(io_service_, tcp::endpoint(tcp::v4(), port)), router_(new Router()) {
}

void HttpServer::start() {
    std::cout << "Server is starting on port " << acceptor_.local_endpoint().port() << std::endl;
    do_accept();
    io_service_.run();
}

void HttpServer::stop() {
    io_service_.stop();
    std::cout << "Server has stopped." << std::endl;
}

void HttpServer::do_accept() {
    auto socket = std::make_shared<tcp::socket>(io_service_);
    acceptor_.async_accept(*socket, [this, socket](const boost::system::error_code& error) {
        if (!error) {
            std::cout << "New connection accepted." << std::endl;
            do_read(socket);
        }
        do_accept();
    });
}

void HttpServer::do_read(std::shared_ptr<tcp::socket> socket) {
    auto buffer = std::make_shared<boost::asio::streambuf>();
    boost::asio::async_read_until(*socket, *buffer, "\r\n", [this, socket, buffer](const boost::system::error_code& error, std::size_t bytes_transferred) {
        if (!error) {
            std::istream stream(buffer.get());
            std::string request_line;
            std::getline(stream, request_line);
            std::cout << "Received request: " << request_line << std::endl;

            std::string response = router_->route(request_line);
            do_write(socket, response);
        }
    });
}

void HttpServer::do_write(std::shared_ptr<tcp::socket> socket, const std::string& response) {
    boost::asio::async_write(*socket, boost::asio::buffer(response), [socket](const boost::system::error_code& error, std::size_t /*bytes_transferred*/) {
        if (!error) {
            std::cout << "Response sent." << std::endl;
        }
    });
}