#include "execution_simulator.hpp"
#include "../../utils/logging.hpp"
#include "../../utils/exceptions.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

namespace trading {

// ExecutionSimulator implementation

ExecutionSimulator::ExecutionSimulator(
    const SimulationConfig& config,
    std::shared_ptr<IMarketDataProvider> market_data_provider
) : config_(config),
    market_data_provider_(std::move(market_data_provider)),
    gen_(rd_()),
    uniform_dist_(0.0, 1.0),
    latency_dist_(config_.avg_latency_ms, (config_.max_latency_ms - config_.min_latency_ms) / 4.0),
    slippage_dist_(config_.avg_slippage_bps, (config_.max_slippage_bps - config_.min_slippage_bps) / 4.0) {

    log_execution_event("ExecutionSimulator initialized", nullptr);
}

bool ExecutionSimulator::should_execute_order(std::shared_ptr<Order> order) const {
    if (!order) {
        return false;
    }

    // Check market conditions
    if (!is_market_open()) {
        return false;
    }

    if (is_symbol_halted(order->get_instrument_symbol())) {
        return false;
    }

    // Check fill rates based on order type
    double fill_rate = (order->get_type() == OrderType::MARKET) ?
                      config_.market_order_fill_rate :
                      config_.limit_order_fill_rate;

    return uniform_dist_(gen_) < fill_rate;
}

bool ExecutionSimulator::should_reject_order(std::shared_ptr<Order> order, std::string& rejection_reason) const {
    if (!order) {
        rejection_reason = "Invalid order";
        return true;
    }

    // Random rejection based on configured rate
    if (uniform_dist_(gen_) < config_.rejection_rate) {
        if (!config_.rejection_reasons.empty()) {
            std::uniform_int_distribution<size_t> reason_dist(0, config_.rejection_reasons.size() - 1);
            rejection_reason = config_.rejection_reasons[reason_dist(gen_)];
        } else {
            rejection_reason = "Order rejected by execution simulator";
        }
        return true;
    }

    // Check specific rejection conditions
    if (!is_market_open()) {
        rejection_reason = "Market closed";
        return true;
    }

    if (is_symbol_halted(order->get_instrument_symbol())) {
        rejection_reason = "Symbol halted";
        return true;
    }

    // Check price validity for limit orders
    if (order->get_type() == OrderType::LIMIT) {
        double market_price = get_market_price(order->get_instrument_symbol(), order->get_side());
        if (market_price > 0) {
            double price_diff_pct = std::abs(order->get_price() - market_price) / market_price;
            if (price_diff_pct > 0.1) { // Reject if price is more than 10% away from market
                rejection_reason = "Price too far from market";
                return true;
            }
        }
    }

    return false;
}

std::vector<ExecutionSimulator::ExecutionResult> ExecutionSimulator::simulate_execution(std::shared_ptr<Order> order) {
    std::vector<ExecutionResult> results;

    if (!order) {
        return results;
    }

    // Check if order should be rejected
    std::string rejection_reason;
    if (should_reject_order(order, rejection_reason)) {
        ExecutionResult result;
        result.should_execute = false;
        result.rejection_reason = rejection_reason;
        update_execution_stats(result);
        results.push_back(result);
        return results;
    }

    // Check if order should execute
    if (!should_execute_order(order)) {
        return results; // No execution, order remains working
    }

    // Determine if this will be a partial fill
    bool is_partial = should_partially_fill();
    double remaining_quantity = order->get_remaining_quantity();

    while (remaining_quantity > 0) {
        ExecutionResult result;
        result.should_execute = true;
        result.is_partial_fill = is_partial && (remaining_quantity > 0);

        // Calculate execution quantity
        if (is_partial) {
            result.executed_quantity = calculate_partial_fill_quantity(remaining_quantity);
        } else {
            result.executed_quantity = remaining_quantity;
        }

        // Simulate execution price
        double market_price = get_market_price(order->get_instrument_symbol(), order->get_side());
        result.execution_price = simulate_execution_price(order, market_price);

        // Simulate latency
        result.latency = simulate_execution_latency();

        update_execution_stats(result);
        results.push_back(result);

        remaining_quantity -= result.executed_quantity;

        // For limit orders, only execute once
        if (order->get_type() == OrderType::LIMIT) {
            break;
        }

        // For partial fills, break after first execution in this simulation
        if (is_partial) {
            break;
        }
    }

    return results;
}

double ExecutionSimulator::simulate_execution_price(std::shared_ptr<Order> order, double market_price) const {
    if (market_price <= 0) {
        // Fallback price if market data is not available
        market_price = order->get_type() == OrderType::LIMIT ? order->get_price() : 100.0;
    }

    double execution_price = market_price;

    // For limit orders, execution price cannot be worse than limit price
    if (order->get_type() == OrderType::LIMIT) {
        if (order->get_side() == OrderSide::BUY) {
            execution_price = std::min(market_price, order->get_price());
        } else {
            execution_price = std::max(market_price, order->get_price());
        }
    }

    // Apply slippage for market orders
    if (order->get_type() == OrderType::MARKET) {
        double slippage = simulate_slippage(order, execution_price);
        if (order->get_side() == OrderSide::BUY) {
            execution_price += slippage;
        } else {
            execution_price -= slippage;
        }
    }

    // Apply market impact if enabled
    if (config_.simulate_market_impact) {
        double impact = calculate_market_impact(order, execution_price);
        if (order->get_side() == OrderSide::BUY) {
            execution_price += impact;
        } else {
            execution_price -= impact;
        }
    }

    return std::max(0.01, execution_price); // Ensure positive price
}

double ExecutionSimulator::simulate_slippage(std::shared_ptr<Order> order, double base_price) const {
    (void)order; // Suppress unused parameter warning
    double slippage_bps = std::max(config_.min_slippage_bps,
                                  std::min(config_.max_slippage_bps,
                                          slippage_dist_(gen_)));

    return base_price * (slippage_bps / 10000.0);
}

std::chrono::milliseconds ExecutionSimulator::simulate_execution_latency() const {
    double latency_ms = std::max(config_.min_latency_ms,
                                std::min(config_.max_latency_ms,
                                        latency_dist_(gen_)));

    return std::chrono::milliseconds(static_cast<long long>(latency_ms));
}

void ExecutionSimulator::set_config(const SimulationConfig& config) {
    config_ = config;

    // Update distributions
    latency_dist_ = std::normal_distribution<double>(
        config_.avg_latency_ms,
        (config_.max_latency_ms - config_.min_latency_ms) / 4.0
    );

    slippage_dist_ = std::normal_distribution<double>(
        config_.avg_slippage_bps,
        (config_.max_slippage_bps - config_.min_slippage_bps) / 4.0
    );
}

const ExecutionSimulator::SimulationConfig& ExecutionSimulator::get_config() const {
    return config_;
}

void ExecutionSimulator::set_market_data_provider(std::shared_ptr<IMarketDataProvider> provider) {
    market_data_provider_ = std::move(provider);
}

ExecutionSimulator::ExecutionStats ExecutionSimulator::get_statistics() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void ExecutionSimulator::reset_statistics() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = ExecutionStats{};
}

// Private helper methods

double ExecutionSimulator::get_market_price(const std::string& symbol, OrderSide side) const {
    if (!market_data_provider_) {
        return 0.0;
    }

    auto tick = market_data_provider_->get_latest_tick(symbol);
    if (!tick) {
        return 0.0;
    }

    // Return appropriate price based on order side
    if (side == OrderSide::BUY) {
        return tick->ask_price;
    } else {
        return tick->bid_price;
    }
}

bool ExecutionSimulator::is_market_open() const {
    // Simple implementation - assume market is always open for simulation
    // In a real system, this would check actual market hours
    return true;
}

bool ExecutionSimulator::is_symbol_halted(const std::string& symbol) const {
    // Simple implementation - assume no symbols are halted for simulation
    // In a real system, this would check symbol halt status
    (void)symbol; // Suppress unused parameter warning
    return false;
}

bool ExecutionSimulator::should_partially_fill() const {
    return uniform_dist_(gen_) < config_.partial_fill_probability;
}

double ExecutionSimulator::calculate_partial_fill_quantity(double total_quantity) const {
    // Fill between 10% and 90% of remaining quantity
    std::uniform_real_distribution<double> fill_dist(0.1, 0.9);
    double fill_ratio = fill_dist(gen_);
    return std::max(1.0, total_quantity * fill_ratio);
}

double ExecutionSimulator::calculate_market_impact(std::shared_ptr<Order> order, double base_price) const {
    // Simple market impact model based on order size
    double impact_factor = config_.impact_factor;
    double quantity_factor = std::log10(order->get_remaining_quantity() / 100.0); // Log scale
    double impact = base_price * impact_factor * quantity_factor * 0.001; // 0.1% per log10(quantity/100)

    return std::max(0.0, impact);
}

void ExecutionSimulator::update_execution_stats(const ExecutionResult& result) const {
    std::lock_guard<std::mutex> lock(stats_mutex_);

    stats_.total_orders++;

    if (result.should_execute) {
        stats_.executed_orders++;
        if (result.is_partial_fill) {
            stats_.partial_fills++;
        }

        // Update average latency
        double new_latency = result.latency.count();
        stats_.avg_latency_ms = (stats_.avg_latency_ms * (stats_.executed_orders - 1) + new_latency) / stats_.executed_orders;

        // Update average slippage (simplified calculation)
        // In a real implementation, this would require the original market price
    } else {
        stats_.rejected_orders++;
    }

    // Update fill rate
    stats_.fill_rate = static_cast<double>(stats_.executed_orders) / stats_.total_orders;
}

void ExecutionSimulator::log_execution_event(const std::string& event, std::shared_ptr<Order> order) const {
    if (order) {
        Logger::info("ExecutionSimulator: " + event + " - Order ID: " + order->get_order_id() +
                     ", Symbol: " + order->get_instrument_symbol());
    } else {
        Logger::info("ExecutionSimulator: " + event);
    }
}

// MarketConditionSimulator implementation

MarketConditionSimulator::MarketConditionSimulator()
    : current_condition_(MarketCondition::NORMAL),
      random_changes_enabled_(false),
      gen_(rd_()),
      uniform_dist_(0.0, 1.0),
      last_condition_change_(std::chrono::system_clock::now()) {
}

void MarketConditionSimulator::set_market_condition(MarketCondition condition) {
    current_condition_ = condition;
    last_condition_change_ = std::chrono::system_clock::now();
}

MarketConditionSimulator::MarketCondition MarketConditionSimulator::get_current_condition() const {
    return current_condition_;
}

double MarketConditionSimulator::get_liquidity_multiplier() const {
    switch (current_condition_) {
        case MarketCondition::NORMAL: return 1.0;
        case MarketCondition::VOLATILE: return 0.8;
        case MarketCondition::ILLIQUID: return 0.3;
        case MarketCondition::TRENDING_UP: return 1.2;
        case MarketCondition::TRENDING_DOWN: return 1.2;
        case MarketCondition::GAPPING: return 0.5;
        case MarketCondition::HALTED: return 0.0;
        default: return 1.0;
    }
}

double MarketConditionSimulator::get_volatility_multiplier() const {
    switch (current_condition_) {
        case MarketCondition::NORMAL: return 1.0;
        case MarketCondition::VOLATILE: return 3.0;
        case MarketCondition::ILLIQUID: return 1.5;
        case MarketCondition::TRENDING_UP: return 0.8;
        case MarketCondition::TRENDING_DOWN: return 0.8;
        case MarketCondition::GAPPING: return 5.0;
        case MarketCondition::HALTED: return 0.0;
        default: return 1.0;
    }
}

double MarketConditionSimulator::get_slippage_multiplier() const {
    switch (current_condition_) {
        case MarketCondition::NORMAL: return 1.0;
        case MarketCondition::VOLATILE: return 2.5;
        case MarketCondition::ILLIQUID: return 4.0;
        case MarketCondition::TRENDING_UP: return 1.2;
        case MarketCondition::TRENDING_DOWN: return 1.2;
        case MarketCondition::GAPPING: return 10.0;
        case MarketCondition::HALTED: return 0.0;
        default: return 1.0;
    }
}

double MarketConditionSimulator::get_rejection_rate_multiplier() const {
    switch (current_condition_) {
        case MarketCondition::NORMAL: return 1.0;
        case MarketCondition::VOLATILE: return 2.0;
        case MarketCondition::ILLIQUID: return 5.0;
        case MarketCondition::TRENDING_UP: return 0.5;
        case MarketCondition::TRENDING_DOWN: return 0.5;
        case MarketCondition::GAPPING: return 8.0;
        case MarketCondition::HALTED: return 100.0;
        default: return 1.0;
    }
}

void MarketConditionSimulator::enable_random_condition_changes(bool enable) {
    random_changes_enabled_ = enable;
}

void MarketConditionSimulator::update_market_conditions() {
    if (!random_changes_enabled_) {
        return;
    }

    auto now = std::chrono::system_clock::now();
    if (now - last_condition_change_ < condition_change_interval_) {
        return;
    }

    // 20% chance to change condition
    if (uniform_dist_(gen_) < 0.2) {
        current_condition_ = generate_random_condition();
        last_condition_change_ = now;
    }
}

MarketConditionSimulator::MarketCondition MarketConditionSimulator::generate_random_condition() const {
    std::uniform_int_distribution<int> condition_dist(0, 6);
    return static_cast<MarketCondition>(condition_dist(gen_));
}

// ExecutionReplaySystem implementation

ExecutionReplaySystem::ExecutionReplaySystem()
    : replay_index_(0),
      replay_speed_(1.0),
      is_recording_(false),
      is_replaying_(false) {
}

void ExecutionReplaySystem::start_recording() {
    std::lock_guard<std::mutex> lock(replay_mutex_);
    recorded_executions_.clear();
    is_recording_ = true;
    recording_start_time_ = std::chrono::system_clock::now();
}

void ExecutionReplaySystem::stop_recording() {
    std::lock_guard<std::mutex> lock(replay_mutex_);
    is_recording_ = false;
}

void ExecutionReplaySystem::record_execution(const ExecutionRecord& record) {
    std::lock_guard<std::mutex> lock(replay_mutex_);
    if (is_recording_) {
        recorded_executions_.push_back(record);
    }
}

bool ExecutionReplaySystem::load_recording(const std::string& filename) {
    // Implementation would load from file (JSON, CSV, etc.)
    // For now, just return false
    (void)filename;
    return false;
}

bool ExecutionReplaySystem::save_recording(const std::string& filename) const {
    // Implementation would save to file (JSON, CSV, etc.)
    // For now, just return false
    (void)filename;
    return false;
}

void ExecutionReplaySystem::start_replay() {
    std::lock_guard<std::mutex> lock(replay_mutex_);
    replay_index_ = 0;
    is_replaying_ = true;
    replay_start_time_ = std::chrono::system_clock::now();
}

void ExecutionReplaySystem::stop_replay() {
    std::lock_guard<std::mutex> lock(replay_mutex_);
    is_replaying_ = false;
}

bool ExecutionReplaySystem::get_next_execution(ExecutionRecord& record) {
    std::lock_guard<std::mutex> lock(replay_mutex_);

    if (!is_replaying_ || replay_index_ >= recorded_executions_.size()) {
        return false;
    }

    record = recorded_executions_[replay_index_++];
    return true;
}

std::vector<ExecutionReplaySystem::ExecutionRecord> ExecutionReplaySystem::get_all_records() const {
    std::lock_guard<std::mutex> lock(replay_mutex_);
    return recorded_executions_;
}

ExecutionSimulator::ExecutionStats ExecutionReplaySystem::analyze_recording() const {
    std::lock_guard<std::mutex> lock(replay_mutex_);

    ExecutionSimulator::ExecutionStats stats;
    stats.total_orders = recorded_executions_.size();

    double total_latency = 0.0;
    for (const auto& record : recorded_executions_) {
        if (!record.was_rejected) {
            stats.executed_orders++;
            total_latency += record.latency.count();

            if (record.executed_quantity < record.quantity) {
                stats.partial_fills++;
            }
        } else {
            stats.rejected_orders++;
        }
    }

    if (stats.executed_orders > 0) {
        stats.avg_latency_ms = total_latency / stats.executed_orders;
        stats.fill_rate = static_cast<double>(stats.executed_orders) / stats.total_orders;
    }

    return stats;
}

void ExecutionReplaySystem::set_replay_speed(double speed_multiplier) {
    replay_speed_ = std::max(0.1, speed_multiplier);
}

// ExecutionBenchmark implementation

ExecutionBenchmark::ExecutionBenchmark(std::shared_ptr<ExecutionSimulator> simulator)
    : simulator_(std::move(simulator)),
      gen_(rd_()) {
}

void ExecutionBenchmark::run_benchmark(const BenchmarkConfig& config) {
    if (!simulator_) {
        return;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    BenchmarkResults results;
    results.total_orders = config.num_orders;

    std::uniform_real_distribution<double> quantity_dist(config.min_quantity, config.max_quantity);
    std::uniform_int_distribution<size_t> symbol_dist(0, config.symbols.size() - 1);
    std::uniform_real_distribution<double> order_type_dist(0.0, 1.0);
    std::uniform_real_distribution<double> side_dist(0.0, 1.0);

    double total_latency = 0.0;

    for (size_t i = 0; i < config.num_orders; ++i) {
        auto order = generate_random_order(config);

        std::string rejection_reason;
        if (simulator_->should_reject_order(order, rejection_reason)) {
            results.rejected_orders++;
        } else if (simulator_->should_execute_order(order)) {
            results.successful_executions++;

            auto latency = simulator_->simulate_execution_latency();
            total_latency += latency.count();
        }

        // Simulate order submission rate
        if (config.orders_per_second > 0) {
            auto delay = std::chrono::microseconds(static_cast<long long>(1000000.0 / config.orders_per_second));
            std::this_thread::sleep_for(delay);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    results.total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    if (results.successful_executions > 0) {
        results.avg_execution_latency_ms = total_latency / results.successful_executions;
    }

    results.fill_rate = static_cast<double>(results.successful_executions) / results.total_orders;
    results.avg_orders_per_second = (results.total_duration.count() > 0) ?
        (results.total_orders * 1000.0) / results.total_duration.count() : 0.0;

    last_results_ = results;
    log_benchmark_results(results);
}

ExecutionBenchmark::BenchmarkResults ExecutionBenchmark::get_last_results() const {
    return last_results_;
}

bool ExecutionBenchmark::validate_execution_times() const {
    // Validate that execution times are within acceptable ranges
    return last_results_.avg_execution_latency_ms < 100.0; // Less than 100ms average
}

bool ExecutionBenchmark::validate_fill_rates() const {
    // Validate that fill rates are reasonable
    return last_results_.fill_rate > 0.5; // At least 50% fill rate
}

std::shared_ptr<Order> ExecutionBenchmark::generate_random_order(const BenchmarkConfig& config) const {
    // Generate random order parameters
    std::uniform_real_distribution<double> quantity_dist(config.min_quantity, config.max_quantity);
    std::uniform_int_distribution<size_t> symbol_dist(0, config.symbols.size() - 1);
    std::uniform_real_distribution<double> order_type_dist(0.0, 1.0);
    std::uniform_real_distribution<double> side_dist(0.0, 1.0);

    // Generate values for Order constructor
    std::string order_id = "BENCH_" + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    std::string symbol = config.symbols[symbol_dist(gen_)];
    OrderSide side = (side_dist(gen_) < 0.5) ? OrderSide::BUY : OrderSide::SELL;
    OrderType type = (order_type_dist(gen_) < config.market_order_ratio) ? OrderType::MARKET : OrderType::LIMIT;
    double quantity = quantity_dist(gen_);
    double price = (type == OrderType::LIMIT) ? 100.0 : 0.0; // Fixed price for simplicity

    // Create order using constructor
    auto order = std::make_shared<Order>(order_id, symbol, side, type, quantity, price);

    return order;
}

void ExecutionBenchmark::log_benchmark_results(const BenchmarkResults& results) const {
    std::ostringstream oss;
    oss << "Benchmark Results - "
        << "Orders: " << results.total_orders
        << ", Executions: " << results.successful_executions
        << ", Rejections: " << results.rejected_orders
        << ", Fill Rate: " << std::fixed << std::setprecision(2) << (results.fill_rate * 100.0) << "%"
        << ", Avg Latency: " << results.avg_execution_latency_ms << "ms"
        << ", Orders/sec: " << results.avg_orders_per_second;

    Logger::info("ExecutionBenchmark: " + oss.str());
}

} // namespace trading