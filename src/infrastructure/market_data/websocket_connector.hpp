#pragma once

#include "core/models/market_tick.hpp"
#include "utils/logging.hpp"
#include "utils/exceptions.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <vector>

namespace trading {

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

/**
 * WebSocket connector for real-time market data
 * Features:
 * - Asynchronous connection management
 * - Automatic reconnection with exponential backoff
 * - JSON message parsing
 * - Subscription management
 * - Thread-safe operations
 */
class WebSocketConnector {
public:
    // Callback function types
    using TickCallback = std::function<void(const MarketTick&)>;
    using ConnectionCallback = std::function<void(bool connected)>;
    using ErrorCallback = std::function<void(const std::string& error)>;

    // Constructor
    WebSocketConnector(const std::string& host, const std::string& port, const std::string& target);

    // Destructor
    ~WebSocketConnector();

    // Connection management
    void connect_async();
    void disconnect();
    bool is_connected() const;

    // Subscription management
    void subscribe(const std::string& symbol);
    void unsubscribe(const std::string& symbol);
    std::vector<std::string> get_subscribed_symbols() const;

    // Callback setters
    void set_tick_callback(TickCallback callback);
    void set_connection_callback(ConnectionCallback callback);
    void set_error_callback(ErrorCallback callback);

    // Configuration
    void set_reconnect_enabled(bool enabled) { auto_reconnect_ = enabled; }
    void set_heartbeat_interval(std::chrono::seconds interval) { heartbeat_interval_ = interval; }

private:
    // Network details
    std::string host_;
    std::string port_;
    std::string target_;

    // ASIO components
    net::io_context ioc_;
    tcp::resolver resolver_;
    websocket::stream<beast::tcp_stream> ws_;
    net::executor_work_guard<net::io_context::executor_type> work_guard_;

    // Threading
    std::thread io_thread_;
    std::atomic<bool> running_;
    std::atomic<bool> connected_;

    // Reconnection
    std::atomic<bool> auto_reconnect_;
    std::chrono::seconds reconnect_delay_;
    std::chrono::seconds heartbeat_interval_;
    net::steady_timer reconnect_timer_;
    net::steady_timer heartbeat_timer_;

    // Subscriptions
    mutable std::mutex subscriptions_mutex_;
    std::vector<std::string> subscribed_symbols_;

    // Callbacks
    TickCallback tick_callback_;
    ConnectionCallback connection_callback_;
    ErrorCallback error_callback_;

    // Message handling
    beast::flat_buffer buffer_;

    // Private methods
    void run_io_context();
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);
    void on_handshake(beast::error_code ec);
    void on_write(beast::error_code ec, std::size_t bytes_transferred);
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_close(beast::error_code ec);

    // Reconnection logic
    void schedule_reconnect();
    void attempt_reconnect();

    // Heartbeat
    void start_heartbeat();
    void send_heartbeat();

    // Message processing
    void process_message(const std::string& message);
    MarketTick parse_market_tick(const std::string& json_message);
    std::string create_subscribe_message(const std::string& symbol);
    std::string create_unsubscribe_message(const std::string& symbol);

    // Utility
    void notify_connection_status(bool connected);
    void notify_error(const std::string& error);
    void start_reading();
    void close_connection();
};

} // namespace trading