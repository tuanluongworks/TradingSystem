#pragma once

#include "../../contracts/trading_engine_api.hpp"
#include "../models/market_tick.hpp"
#include "../messaging/message_queue.hpp"
#include "websocket_connector.hpp"

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>
#include <chrono>
#include <random>

namespace trading {

// Forward declarations
class WebSocketConnector;

/**
 * Market Data Provider Implementation
 * Manages real-time market data connections, subscriptions, and distribution
 */
class MarketDataProvider : public IMarketDataProvider {
public:
    enum class ProviderMode {
        SIMULATION,     // Generate simulated market data
        WEBSOCKET,      // Real WebSocket market data
        FILE_REPLAY     // Replay from historical files
    };

    struct ProviderConfig {
        ProviderMode mode = ProviderMode::SIMULATION;
        std::string websocket_url;
        std::string api_key;
        int max_ticks_per_symbol = 1000;
        int update_interval_ms = 100;
        double simulation_volatility = 0.02;  // 2% volatility for simulation
        std::vector<std::string> default_symbols = {"AAPL", "GOOGL", "MSFT", "TSLA", "AMZN"};
    };

    explicit MarketDataProvider(const ProviderConfig& config = {});
    virtual ~MarketDataProvider();

    // IMarketDataProvider implementation
    bool connect() override;
    void disconnect() override;
    bool is_connected() const override;

    bool subscribe(const std::string& symbol) override;
    bool unsubscribe(const std::string& symbol) override;
    std::vector<std::string> get_subscribed_symbols() const override;

    std::shared_ptr<MarketTick> get_latest_tick(const std::string& symbol) const override;
    std::vector<std::shared_ptr<MarketTick>> get_recent_ticks(const std::string& symbol, int count) const override;

    void set_tick_callback(std::function<void(const MarketTick&)> callback) override;
    void set_connection_callback(std::function<void(bool connected)> callback) override;

    // Additional functionality
    void start_data_generation();
    void stop_data_generation();

    // Configuration
    void set_update_interval(int interval_ms);
    void set_simulation_params(double volatility, double base_price = 100.0);

    // Statistics
    size_t get_total_tick_count() const;
    size_t get_subscription_count() const;
    std::chrono::system_clock::time_point get_last_update() const;

    // Health check
    bool is_healthy() const;
    std::string get_status() const;

private:
    // Configuration
    ProviderConfig config_;

    // Connection state
    std::atomic<bool> is_connected_;
    std::atomic<bool> is_running_;
    std::atomic<bool> should_stop_;

    // Thread management
    std::thread data_generation_thread_;
    std::thread websocket_thread_;
    mutable std::mutex provider_mutex_;

    // Data storage
    std::unordered_map<std::string, std::vector<std::shared_ptr<MarketTick>>> tick_history_;
    std::unordered_map<std::string, std::shared_ptr<MarketTick>> latest_ticks_;
    std::unordered_set<std::string> subscribed_symbols_;

    // Callbacks
    std::function<void(const MarketTick&)> tick_callback_;
    std::function<void(bool)> connection_callback_;

    // WebSocket integration
    std::unique_ptr<WebSocketConnector> websocket_connector_;

    // Simulation state
    std::unordered_map<std::string, double> current_prices_;
    std::random_device random_device_;
    std::mt19937 random_generator_;
    std::normal_distribution<double> price_distribution_;

    // Statistics
    std::atomic<size_t> total_tick_count_;
    std::atomic<std::chrono::system_clock::time_point> last_update_;

    // Internal methods
    void initialize_simulation();
    void data_generation_loop();
    void generate_simulated_tick(const std::string& symbol);

    // WebSocket methods
    void setup_websocket_connection();
    void on_websocket_message(const std::string& message);
    void on_websocket_connection_change(bool connected);

    // Data management
    void store_tick(std::shared_ptr<MarketTick> tick);
    void cleanup_old_ticks();
    void notify_tick(const MarketTick& tick);
    void notify_connection_change(bool connected);

    // Utility methods
    double calculate_next_price(const std::string& symbol, double current_price);
    std::shared_ptr<MarketTick> create_tick(
        const std::string& symbol,
        double bid,
        double ask,
        double last,
        double volume = 1000.0
    );

    // Validation
    bool is_valid_symbol(const std::string& symbol) const;
    bool is_symbol_subscribed(const std::string& symbol) const;

    // Logging
    void log_provider_event(const std::string& event) const;
};

/**
 * Market Data Simulator
 * Helper class for generating realistic simulated market data
 */
class MarketDataSimulator {
public:
    struct SimulationParams {
        double base_price = 100.0;
        double volatility = 0.02;           // Daily volatility
        double mean_reversion = 0.1;        // Mean reversion strength
        double tick_size = 0.01;            // Minimum price increment
        double spread_bps = 5.0;            // Bid-ask spread in basis points
        double volume_mean = 1000.0;        // Average volume per tick
        double volume_std = 200.0;          // Volume standard deviation
    };

    explicit MarketDataSimulator(const SimulationParams& params = {});

    // Generate next price based on random walk with mean reversion
    double generate_next_price(const std::string& symbol, double current_price);

    // Generate realistic bid/ask spread
    std::pair<double, double> generate_bid_ask(double mid_price);

    // Generate volume
    double generate_volume();

    // Set simulation parameters
    void set_params(const SimulationParams& params);
    const SimulationParams& get_params() const;

private:
    SimulationParams params_;
    std::unordered_map<std::string, double> target_prices_;  // Mean reversion targets
    std::random_device rd_;
    mutable std::mt19937 gen_;
    mutable std::normal_distribution<double> price_dist_;
    mutable std::normal_distribution<double> volume_dist_;
    mutable std::uniform_real_distribution<double> uniform_dist_;
};

/**
 * Market Data Cache
 * Efficient storage and retrieval of market data
 */
class MarketDataCache {
public:
    explicit MarketDataCache(size_t max_ticks_per_symbol = 1000);

    // Data operations
    void store_tick(std::shared_ptr<MarketTick> tick);
    std::shared_ptr<MarketTick> get_latest_tick(const std::string& symbol) const;
    std::vector<std::shared_ptr<MarketTick>> get_recent_ticks(
        const std::string& symbol,
        int count
    ) const;

    // Management
    void clear_symbol(const std::string& symbol);
    void clear_all();
    size_t get_tick_count(const std::string& symbol) const;
    size_t get_total_tick_count() const;

    // Cleanup
    void cleanup_old_ticks(std::chrono::hours max_age = std::chrono::hours(24));

private:
    struct SymbolData {
        std::vector<std::shared_ptr<MarketTick>> ticks;
        size_t max_size;

        explicit SymbolData(size_t max_sz) : max_size(max_sz) {
            ticks.reserve(max_sz);
        }
    };

    mutable std::mutex cache_mutex_;
    std::unordered_map<std::string, SymbolData> symbol_data_;
    size_t max_ticks_per_symbol_;

    void maintain_size_limit(SymbolData& data);
};

} // namespace trading