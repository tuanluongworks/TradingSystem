#include "market_data_provider.hpp"
#include "../../utils/logging.hpp"
#include "../../utils/exceptions.hpp"

#include <nlohmann/json.hpp>
#include <sstream>
#include <algorithm>
#include <cmath>

namespace trading {

// MarketDataProvider implementation

MarketDataProvider::MarketDataProvider(const ProviderConfig& config)
    : config_(config),
      is_connected_(false),
      is_running_(false),
      should_stop_(false),
      random_generator_(random_device_()),
      price_distribution_(0.0, 1.0),
      total_tick_count_(0) {

    // Initialize simulation data
    if (config_.mode == ProviderMode::SIMULATION) {
        initialize_simulation();
    }

    log_provider_event("MarketDataProvider initialized in " +
                      (config_.mode == ProviderMode::SIMULATION ? "SIMULATION" : "WEBSOCKET") + " mode");
}

MarketDataProvider::~MarketDataProvider() {
    disconnect();
}

bool MarketDataProvider::connect() {
    std::lock_guard<std::mutex> lock(provider_mutex_);

    if (is_connected_.load()) {
        return true;
    }

    try {
        if (config_.mode == ProviderMode::SIMULATION) {
            // Subscribe to default symbols for simulation
            for (const auto& symbol : config_.default_symbols) {
                subscribed_symbols_.insert(symbol);
                current_prices_[symbol] = 100.0; // Default starting price
            }

            start_data_generation();
            is_connected_.store(true);
            notify_connection_change(true);
            log_provider_event("Connected in simulation mode");

        } else if (config_.mode == ProviderMode::WEBSOCKET) {
            setup_websocket_connection();
            // Connection status will be updated by WebSocket callbacks
        }

        return is_connected_.load();

    } catch (const std::exception& e) {
        log_provider_event("Failed to connect: " + std::string(e.what()));
        return false;
    }
}

void MarketDataProvider::disconnect() {
    if (!is_connected_.load()) {
        return;
    }

    log_provider_event("Disconnecting market data provider");

    stop_data_generation();

    if (websocket_connector_) {
        websocket_connector_.reset();
    }

    {
        std::lock_guard<std::mutex> lock(provider_mutex_);
        subscribed_symbols_.clear();
        current_prices_.clear();
        latest_ticks_.clear();
        tick_history_.clear();
    }

    is_connected_.store(false);
    notify_connection_change(false);

    log_provider_event("Market data provider disconnected");
}

bool MarketDataProvider::is_connected() const {
    return is_connected_.load();
}

bool MarketDataProvider::subscribe(const std::string& symbol) {
    if (!is_valid_symbol(symbol)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(provider_mutex_);

    if (subscribed_symbols_.find(symbol) != subscribed_symbols_.end()) {
        return true; // Already subscribed
    }

    subscribed_symbols_.insert(symbol);

    if (config_.mode == ProviderMode::SIMULATION) {
        // Initialize price for new symbol
        current_prices_[symbol] = 100.0;
        log_provider_event("Subscribed to " + symbol + " (simulation)");
    } else if (websocket_connector_) {
        // Send subscription message via WebSocket
        // Implementation depends on specific WebSocket protocol
        log_provider_event("Subscribed to " + symbol + " (websocket)");
    }

    return true;
}

bool MarketDataProvider::unsubscribe(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(provider_mutex_);

    auto it = subscribed_symbols_.find(symbol);
    if (it == subscribed_symbols_.end()) {
        return false; // Not subscribed
    }

    subscribed_symbols_.erase(it);

    if (config_.mode == ProviderMode::SIMULATION) {
        current_prices_.erase(symbol);
    }

    // Clean up data
    latest_ticks_.erase(symbol);
    tick_history_.erase(symbol);

    log_provider_event("Unsubscribed from " + symbol);
    return true;
}

std::vector<std::string> MarketDataProvider::get_subscribed_symbols() const {
    std::lock_guard<std::mutex> lock(provider_mutex_);
    return std::vector<std::string>(subscribed_symbols_.begin(), subscribed_symbols_.end());
}

std::shared_ptr<MarketTick> MarketDataProvider::get_latest_tick(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(provider_mutex_);
    auto it = latest_ticks_.find(symbol);
    return (it != latest_ticks_.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<MarketTick>> MarketDataProvider::get_recent_ticks(const std::string& symbol, int count) const {
    std::lock_guard<std::mutex> lock(provider_mutex_);

    auto it = tick_history_.find(symbol);
    if (it == tick_history_.end()) {
        return {};
    }

    const auto& ticks = it->second;
    if (ticks.empty()) {
        return {};
    }

    size_t start_idx = (ticks.size() > static_cast<size_t>(count)) ?
                       ticks.size() - count : 0;

    return std::vector<std::shared_ptr<MarketTick>>(
        ticks.begin() + start_idx,
        ticks.end()
    );
}

void MarketDataProvider::set_tick_callback(std::function<void(const MarketTick&)> callback) {
    tick_callback_ = std::move(callback);
}

void MarketDataProvider::set_connection_callback(std::function<void(bool)> callback) {
    connection_callback_ = std::move(callback);
}

void MarketDataProvider::start_data_generation() {
    if (is_running_.load()) {
        return;
    }

    should_stop_.store(false);
    is_running_.store(true);

    data_generation_thread_ = std::thread(&MarketDataProvider::data_generation_loop, this);
    log_provider_event("Started data generation thread");
}

void MarketDataProvider::stop_data_generation() {
    if (!is_running_.load()) {
        return;
    }

    should_stop_.store(true);

    if (data_generation_thread_.joinable()) {
        data_generation_thread_.join();
    }

    is_running_.store(false);
    log_provider_event("Stopped data generation thread");
}

void MarketDataProvider::set_update_interval(int interval_ms) {
    config_.update_interval_ms = interval_ms;
}

void MarketDataProvider::set_simulation_params(double volatility, double base_price) {
    config_.simulation_volatility = volatility;

    std::lock_guard<std::mutex> lock(provider_mutex_);
    for (auto& [symbol, price] : current_prices_) {
        price = base_price;
    }
}

size_t MarketDataProvider::get_total_tick_count() const {
    return total_tick_count_.load();
}

size_t MarketDataProvider::get_subscription_count() const {
    std::lock_guard<std::mutex> lock(provider_mutex_);
    return subscribed_symbols_.size();
}

std::chrono::system_clock::time_point MarketDataProvider::get_last_update() const {
    return last_update_.load();
}

bool MarketDataProvider::is_healthy() const {
    if (!is_connected_.load()) {
        return false;
    }

    auto now = std::chrono::system_clock::now();
    auto last_update = last_update_.load();
    auto time_since_update = std::chrono::duration_cast<std::chrono::seconds>(now - last_update);

    // Consider healthy if we received data within the last 10 seconds
    return time_since_update.count() < 10;
}

std::string MarketDataProvider::get_status() const {
    std::ostringstream oss;
    oss << "Connected: " << (is_connected_.load() ? "Yes" : "No")
        << ", Mode: " << (config_.mode == ProviderMode::SIMULATION ? "SIMULATION" : "WEBSOCKET")
        << ", Subscriptions: " << get_subscription_count()
        << ", Total Ticks: " << get_total_tick_count()
        << ", Healthy: " << (is_healthy() ? "Yes" : "No");
    return oss.str();
}

// Private methods

void MarketDataProvider::initialize_simulation() {
    // Initialize random number generation for market simulation
    price_distribution_ = std::normal_distribution<double>(0.0, config_.simulation_volatility);
}

void MarketDataProvider::data_generation_loop() {
    while (!should_stop_.load()) {
        try {
            std::vector<std::string> symbols_to_update;
            {
                std::lock_guard<std::mutex> lock(provider_mutex_);
                symbols_to_update.assign(subscribed_symbols_.begin(), subscribed_symbols_.end());
            }

            for (const auto& symbol : symbols_to_update) {
                generate_simulated_tick(symbol);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(config_.update_interval_ms));

        } catch (const std::exception& e) {
            log_provider_event("Error in data generation: " + std::string(e.what()));
        }
    }
}

void MarketDataProvider::generate_simulated_tick(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(provider_mutex_);

    auto price_it = current_prices_.find(symbol);
    if (price_it == current_prices_.end()) {
        return;
    }

    double current_price = price_it->second;
    double new_price = calculate_next_price(symbol, current_price);

    // Generate bid/ask spread (typically 0.05% to 0.10%)
    double spread_pct = 0.0005 + (price_distribution_(random_generator_) * 0.0005);
    double spread = new_price * spread_pct;

    double bid_price = new_price - spread / 2.0;
    double ask_price = new_price + spread / 2.0;

    // Generate volume
    std::uniform_real_distribution<double> volume_dist(500.0, 2000.0);
    double volume = volume_dist(random_generator_);

    // Create tick
    auto tick = create_tick(symbol, bid_price, ask_price, new_price, volume);

    // Update current price
    current_prices_[symbol] = new_price;

    // Store and notify
    store_tick(tick);
    notify_tick(*tick);
}

void MarketDataProvider::setup_websocket_connection() {
    // TODO: Implement WebSocket connection setup
    // This would create and configure the WebSocketConnector
    log_provider_event("WebSocket connection setup not yet implemented");
}

void MarketDataProvider::on_websocket_message(const std::string& message) {
    try {
        auto json_msg = nlohmann::json::parse(message);

        // Parse market data from JSON (format depends on data provider)
        std::string symbol = json_msg["symbol"];
        double bid = json_msg["bid"];
        double ask = json_msg["ask"];
        double last = json_msg["last"];
        double volume = json_msg.value("volume", 1000.0);

        auto tick = create_tick(symbol, bid, ask, last, volume);
        store_tick(tick);
        notify_tick(*tick);

    } catch (const std::exception& e) {
        log_provider_event("Error parsing WebSocket message: " + std::string(e.what()));
    }
}

void MarketDataProvider::on_websocket_connection_change(bool connected) {
    is_connected_.store(connected);
    notify_connection_change(connected);

    if (connected) {
        log_provider_event("WebSocket connected");
    } else {
        log_provider_event("WebSocket disconnected");
    }
}

void MarketDataProvider::store_tick(std::shared_ptr<MarketTick> tick) {
    // Update latest tick
    latest_ticks_[tick->instrument_symbol] = tick;

    // Add to history
    auto& history = tick_history_[tick->instrument_symbol];
    history.push_back(tick);

    // Maintain size limit
    if (history.size() > static_cast<size_t>(config_.max_ticks_per_symbol)) {
        history.erase(history.begin());
    }

    // Update statistics
    total_tick_count_.fetch_add(1);
    last_update_.store(tick->timestamp);
}

void MarketDataProvider::cleanup_old_ticks() {
    std::lock_guard<std::mutex> lock(provider_mutex_);

    auto cutoff_time = std::chrono::system_clock::now() - std::chrono::hours(24);

    for (auto& [symbol, ticks] : tick_history_) {
        auto it = std::remove_if(ticks.begin(), ticks.end(),
            [cutoff_time](const std::shared_ptr<MarketTick>& tick) {
                return tick->timestamp < cutoff_time;
            });
        ticks.erase(it, ticks.end());
    }
}

void MarketDataProvider::notify_tick(const MarketTick& tick) {
    if (tick_callback_) {
        tick_callback_(tick);
    }
}

void MarketDataProvider::notify_connection_change(bool connected) {
    if (connection_callback_) {
        connection_callback_(connected);
    }
}

double MarketDataProvider::calculate_next_price(const std::string& symbol, double current_price) {
    // Simple random walk with mean reversion
    double price_change = price_distribution_(random_generator_) * current_price;

    // Add some mean reversion towards 100.0
    double mean_reversion = (100.0 - current_price) * 0.001;

    double new_price = current_price + price_change + mean_reversion;

    // Ensure price stays positive and reasonable
    return std::max(1.0, std::min(1000.0, new_price));
}

std::shared_ptr<MarketTick> MarketDataProvider::create_tick(
    const std::string& symbol,
    double bid,
    double ask,
    double last,
    double volume
) {
    auto tick = std::make_shared<MarketTick>();
    tick->instrument_symbol = symbol;
    tick->bid_price = bid;
    tick->ask_price = ask;
    tick->last_price = last;
    tick->volume = volume;
    tick->timestamp = std::chrono::system_clock::now();
    return tick;
}

bool MarketDataProvider::is_valid_symbol(const std::string& symbol) const {
    return !symbol.empty() && symbol.length() <= 10 &&
           std::all_of(symbol.begin(), symbol.end(),
                      [](char c) { return std::isalnum(c) || c == '.' || c == '-'; });
}

bool MarketDataProvider::is_symbol_subscribed(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(provider_mutex_);
    return subscribed_symbols_.find(symbol) != subscribed_symbols_.end();
}

void MarketDataProvider::log_provider_event(const std::string& event) const {
    Logger::info("MarketDataProvider", event);
}

// MarketDataSimulator implementation

MarketDataSimulator::MarketDataSimulator(const SimulationParams& params)
    : params_(params),
      gen_(rd_()),
      price_dist_(0.0, params_.volatility),
      volume_dist_(params_.volume_mean, params_.volume_std),
      uniform_dist_(0.0, 1.0) {
}

double MarketDataSimulator::generate_next_price(const std::string& symbol, double current_price) {
    // Get or create target price for mean reversion
    auto it = target_prices_.find(symbol);
    if (it == target_prices_.end()) {
        target_prices_[symbol] = params_.base_price;
        it = target_prices_.find(symbol);
    }

    double target_price = it->second;

    // Random walk component
    double random_change = price_dist_(gen_) * current_price;

    // Mean reversion component
    double mean_reversion = (target_price - current_price) * params_.mean_reversion;

    // Calculate new price
    double new_price = current_price + random_change + mean_reversion;

    // Round to tick size
    new_price = std::round(new_price / params_.tick_size) * params_.tick_size;

    // Ensure reasonable bounds
    return std::max(params_.tick_size, std::min(10000.0, new_price));
}

std::pair<double, double> MarketDataSimulator::generate_bid_ask(double mid_price) {
    double spread = mid_price * (params_.spread_bps / 10000.0);
    double half_spread = spread / 2.0;

    double bid = mid_price - half_spread;
    double ask = mid_price + half_spread;

    // Round to tick size
    bid = std::round(bid / params_.tick_size) * params_.tick_size;
    ask = std::round(ask / params_.tick_size) * params_.tick_size;

    return {std::max(params_.tick_size, bid), ask};
}

double MarketDataSimulator::generate_volume() {
    double volume = volume_dist_(gen_);
    return std::max(1.0, volume);
}

void MarketDataSimulator::set_params(const SimulationParams& params) {
    params_ = params;
    price_dist_ = std::normal_distribution<double>(0.0, params_.volatility);
    volume_dist_ = std::normal_distribution<double>(params_.volume_mean, params_.volume_std);
}

const MarketDataSimulator::SimulationParams& MarketDataSimulator::get_params() const {
    return params_;
}

// MarketDataCache implementation

MarketDataCache::MarketDataCache(size_t max_ticks_per_symbol)
    : max_ticks_per_symbol_(max_ticks_per_symbol) {
}

void MarketDataCache::store_tick(std::shared_ptr<MarketTick> tick) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = symbol_data_.find(tick->instrument_symbol);
    if (it == symbol_data_.end()) {
        symbol_data_.emplace(tick->instrument_symbol, SymbolData(max_ticks_per_symbol_));
        it = symbol_data_.find(tick->instrument_symbol);
    }

    it->second.ticks.push_back(tick);
    maintain_size_limit(it->second);
}

std::shared_ptr<MarketTick> MarketDataCache::get_latest_tick(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = symbol_data_.find(symbol);
    if (it == symbol_data_.end() || it->second.ticks.empty()) {
        return nullptr;
    }

    return it->second.ticks.back();
}

std::vector<std::shared_ptr<MarketTick>> MarketDataCache::get_recent_ticks(
    const std::string& symbol,
    int count
) const {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = symbol_data_.find(symbol);
    if (it == symbol_data_.end()) {
        return {};
    }

    const auto& ticks = it->second.ticks;
    if (ticks.empty()) {
        return {};
    }

    size_t start_idx = (ticks.size() > static_cast<size_t>(count)) ?
                       ticks.size() - count : 0;

    return std::vector<std::shared_ptr<MarketTick>>(
        ticks.begin() + start_idx,
        ticks.end()
    );
}

void MarketDataCache::clear_symbol(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    symbol_data_.erase(symbol);
}

void MarketDataCache::clear_all() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    symbol_data_.clear();
}

size_t MarketDataCache::get_tick_count(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = symbol_data_.find(symbol);
    return (it != symbol_data_.end()) ? it->second.ticks.size() : 0;
}

size_t MarketDataCache::get_total_tick_count() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    size_t total = 0;
    for (const auto& [symbol, data] : symbol_data_) {
        total += data.ticks.size();
    }
    return total;
}

void MarketDataCache::cleanup_old_ticks(std::chrono::hours max_age) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto cutoff_time = std::chrono::system_clock::now() - max_age;

    for (auto& [symbol, data] : symbol_data_) {
        auto& ticks = data.ticks;
        auto it = std::remove_if(ticks.begin(), ticks.end(),
            [cutoff_time](const std::shared_ptr<MarketTick>& tick) {
                return tick->timestamp < cutoff_time;
            });
        ticks.erase(it, ticks.end());
    }
}

void MarketDataCache::maintain_size_limit(SymbolData& data) {
    if (data.ticks.size() > data.max_size) {
        data.ticks.erase(data.ticks.begin());
    }
}

} // namespace trading