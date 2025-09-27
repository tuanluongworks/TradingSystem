#include "websocket_connector.hpp"
#include <nlohmann/json.hpp>
#include <iostream>

namespace trading {

WebSocketConnector::WebSocketConnector(const std::string& host, const std::string& port, const std::string& target)
    : host_(host), port_(port), target_(target),
      resolver_(ioc_), ws_(ioc_), work_guard_(net::make_work_guard(ioc_)),
      running_(false), connected_(false), auto_reconnect_(true),
      reconnect_delay_(std::chrono::seconds(5)), heartbeat_interval_(std::chrono::seconds(30)),
      reconnect_timer_(ioc_), heartbeat_timer_(ioc_) {
}

WebSocketConnector::~WebSocketConnector() {
    disconnect();
}

void WebSocketConnector::connect_async() {
    if (running_) {
        return;
    }

    running_ = true;
    io_thread_ = std::thread([this] { run_io_context(); });

    // Start the connection process
    resolver_.async_resolve(
        host_, port_,
        [this](beast::error_code ec, tcp::resolver::results_type results) {
            on_resolve(ec, results);
        });
}

void WebSocketConnector::disconnect() {
    running_ = false;
    auto_reconnect_ = false;

    if (connected_) {
        close_connection();
    }

    ioc_.stop();
    if (io_thread_.joinable()) {
        io_thread_.join();
    }
}

bool WebSocketConnector::is_connected() const {
    return connected_;
}

void WebSocketConnector::subscribe(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);

    auto it = std::find(subscribed_symbols_.begin(), subscribed_symbols_.end(), symbol);
    if (it == subscribed_symbols_.end()) {
        subscribed_symbols_.push_back(symbol);

        if (connected_) {
            std::string message = create_subscribe_message(symbol);
            ws_.async_write(
                net::buffer(message),
                [this](beast::error_code ec, std::size_t bytes_transferred) {
                    on_write(ec, bytes_transferred);
                });
        }
    }
}

void WebSocketConnector::unsubscribe(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);

    auto it = std::find(subscribed_symbols_.begin(), subscribed_symbols_.end(), symbol);
    if (it != subscribed_symbols_.end()) {
        subscribed_symbols_.erase(it);

        if (connected_) {
            std::string message = create_unsubscribe_message(symbol);
            ws_.async_write(
                net::buffer(message),
                [this](beast::error_code ec, std::size_t bytes_transferred) {
                    on_write(ec, bytes_transferred);
                });
        }
    }
}

std::vector<std::string> WebSocketConnector::get_subscribed_symbols() const {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    return subscribed_symbols_;
}

void WebSocketConnector::set_tick_callback(TickCallback callback) {
    tick_callback_ = callback;
}

void WebSocketConnector::set_connection_callback(ConnectionCallback callback) {
    connection_callback_ = callback;
}

void WebSocketConnector::set_error_callback(ErrorCallback callback) {
    error_callback_ = callback;
}

void WebSocketConnector::run_io_context() {
    try {
        ioc_.run();
    } catch (const std::exception& ex) {
        notify_error("IO context error: " + std::string(ex.what()));
    }
}

void WebSocketConnector::on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
    if (ec) {
        notify_error("Resolve failed: " + ec.message());
        if (auto_reconnect_) {
            schedule_reconnect();
        }
        return;
    }

    // Set connection timeout
    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

    // Make the connection
    beast::get_lowest_layer(ws_).async_connect(
        results,
        [this](beast::error_code ec, tcp::resolver::results_type::endpoint_type ep) {
            on_connect(ec, ep);
        });
}

void WebSocketConnector::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type) {
    if (ec) {
        notify_error("Connect failed: " + ec.message());
        if (auto_reconnect_) {
            schedule_reconnect();
        }
        return;
    }

    // Turn off the timeout and set suggested timeout settings for the websocket
    beast::get_lowest_layer(ws_).expires_never();
    ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));

    // Set user agent
    ws_.set_option(websocket::stream_base::decorator(
        [](websocket::request_type& req) {
            req.set(http::field::user_agent, "TradingSystem/1.0");
        }));

    // Perform the websocket handshake
    ws_.async_handshake(
        host_, target_,
        [this](beast::error_code ec) {
            on_handshake(ec);
        });
}

void WebSocketConnector::on_handshake(beast::error_code ec) {
    if (ec) {
        notify_error("Handshake failed: " + ec.message());
        if (auto_reconnect_) {
            schedule_reconnect();
        }
        return;
    }

    connected_ = true;
    notify_connection_status(true);

    // Start reading messages
    start_reading();

    // Start heartbeat
    start_heartbeat();

    // Re-subscribe to all symbols
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    for (const auto& symbol : subscribed_symbols_) {
        std::string message = create_subscribe_message(symbol);
        ws_.async_write(
            net::buffer(message),
            [this](beast::error_code ec, std::size_t bytes_transferred) {
                on_write(ec, bytes_transferred);
            });
    }
}

void WebSocketConnector::on_write(beast::error_code ec, std::size_t) {
    if (ec) {
        notify_error("Write failed: " + ec.message());
        close_connection();
        return;
    }

    // Message sent successfully
}

void WebSocketConnector::on_read(beast::error_code ec, std::size_t) {
    if (ec) {
        notify_error("Read failed: " + ec.message());
        close_connection();
        return;
    }

    // Process the received message
    std::string message = beast::buffers_to_string(buffer_.data());
    buffer_.consume(buffer_.size());

    try {
        process_message(message);
    } catch (const std::exception& ex) {
        notify_error("Message processing failed: " + std::string(ex.what()));
    }

    // Continue reading
    if (connected_) {
        start_reading();
    }
}

void WebSocketConnector::on_close(beast::error_code ec) {
    connected_ = false;
    notify_connection_status(false);

    if (ec) {
        notify_error("Close failed: " + ec.message());
    }

    if (auto_reconnect_ && running_) {
        schedule_reconnect();
    }
}

void WebSocketConnector::schedule_reconnect() {
    if (!running_) return;

    reconnect_timer_.expires_after(reconnect_delay_);
    reconnect_timer_.async_wait([this](beast::error_code ec) {
        if (!ec && running_) {
            attempt_reconnect();
        }
    });
}

void WebSocketConnector::attempt_reconnect() {
    LOG_INFO("Attempting to reconnect to " + host_ + ":" + port_);

    resolver_.async_resolve(
        host_, port_,
        [this](beast::error_code ec, tcp::resolver::results_type results) {
            on_resolve(ec, results);
        });
}

void WebSocketConnector::start_heartbeat() {
    heartbeat_timer_.expires_after(heartbeat_interval_);
    heartbeat_timer_.async_wait([this](beast::error_code ec) {
        if (!ec && connected_) {
            send_heartbeat();
        }
    });
}

void WebSocketConnector::send_heartbeat() {
    nlohmann::json heartbeat_msg;
    heartbeat_msg["type"] = "ping";
    heartbeat_msg["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    std::string message = heartbeat_msg.dump();
    ws_.async_write(
        net::buffer(message),
        [this](beast::error_code ec, std::size_t bytes_transferred) {
            on_write(ec, bytes_transferred);
            if (!ec && connected_) {
                start_heartbeat();
            }
        });
}

void WebSocketConnector::process_message(const std::string& message) {
    try {
        auto json_msg = nlohmann::json::parse(message);

        if (json_msg.contains("type")) {
            std::string type = json_msg["type"];

            if (type == "tick" && tick_callback_) {
                MarketTick tick = parse_market_tick(message);
                tick_callback_(tick);
            } else if (type == "pong") {
                // Heartbeat response - no action needed
            }
        }
    } catch (const std::exception& ex) {
        throw MarketDataException("Failed to parse message: " + std::string(ex.what()));
    }
}

MarketTick WebSocketConnector::parse_market_tick(const std::string& json_message) {
    auto json_msg = nlohmann::json::parse(json_message);

    MarketTick tick;
    tick.instrument_symbol = json_msg.value("symbol", "");
    tick.bid_price = json_msg.value("bid", 0.0);
    tick.ask_price = json_msg.value("ask", 0.0);
    tick.last_price = json_msg.value("last", 0.0);
    tick.volume = json_msg.value("volume", 0.0);
    tick.timestamp = std::chrono::system_clock::now();

    return tick;
}

std::string WebSocketConnector::create_subscribe_message(const std::string& symbol) {
    nlohmann::json msg;
    msg["type"] = "subscribe";
    msg["symbol"] = symbol;
    return msg.dump();
}

std::string WebSocketConnector::create_unsubscribe_message(const std::string& symbol) {
    nlohmann::json msg;
    msg["type"] = "unsubscribe";
    msg["symbol"] = symbol;
    return msg.dump();
}

void WebSocketConnector::notify_connection_status(bool connected) {
    if (connection_callback_) {
        connection_callback_(connected);
    }
}

void WebSocketConnector::notify_error(const std::string& error) {
    LOG_ERROR("WebSocket error: " + error);
    if (error_callback_) {
        error_callback_(error);
    }
}

void WebSocketConnector::start_reading() {
    ws_.async_read(
        buffer_,
        [this](beast::error_code ec, std::size_t bytes_transferred) {
            on_read(ec, bytes_transferred);
        });
}

void WebSocketConnector::close_connection() {
    connected_ = false;

    // Close the WebSocket connection
    ws_.async_close(websocket::close_code::normal,
        [this](beast::error_code ec) {
            on_close(ec);
        });
}

} // namespace trading