#pragma once

#include "../models/order.hpp"
#include "../models/trade.hpp"
#include "../models/market_tick.hpp"
#include "../../infrastructure/market_data/market_data_provider.hpp"

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <random>
#include <mutex>
#include <atomic>

namespace trading {

// Forward declarations
class IMarketDataProvider;

/**
 * Order Execution Simulator
 * Simulates realistic order execution for development and testing
 */
class ExecutionSimulator {
public:
    enum class ExecutionMode {
        IMMEDIATE,      // Execute orders immediately at market price
        REALISTIC,      // Simulate realistic execution with latency and slippage
        AGGRESSIVE,     // More aggressive fills for testing
        CONSERVATIVE    // Conservative fills, higher rejection rate
    };

    struct SimulationConfig {
        ExecutionMode mode = ExecutionMode::REALISTIC;

        // Latency simulation
        double min_latency_ms = 0.5;       // Minimum execution latency
        double max_latency_ms = 5.0;       // Maximum execution latency
        double avg_latency_ms = 2.0;       // Average execution latency

        // Slippage simulation
        double min_slippage_bps = 0.5;     // Minimum slippage (basis points)
        double max_slippage_bps = 10.0;    // Maximum slippage (basis points)
        double avg_slippage_bps = 2.0;     // Average slippage (basis points)

        // Fill probabilities
        double market_order_fill_rate = 0.99;   // 99% market order fill rate
        double limit_order_fill_rate = 0.75;    // 75% limit order fill rate
        double partial_fill_probability = 0.15;  // 15% chance of partial fill

        // Rejection simulation
        double rejection_rate = 0.02;       // 2% order rejection rate
        std::vector<std::string> rejection_reasons = {
            "Insufficient liquidity",
            "Market closed",
            "Symbol halted",
            "Price too far from market"
        };

        // Market impact
        bool simulate_market_impact = true;
        double impact_factor = 0.1;         // Market impact factor
    };

    explicit ExecutionSimulator(
        const SimulationConfig& config,
        std::shared_ptr<IMarketDataProvider> market_data_provider = nullptr
    );

    virtual ~ExecutionSimulator() = default;

    // Core simulation methods
    bool should_execute_order(std::shared_ptr<Order> order) const;
    bool should_reject_order(std::shared_ptr<Order> order, std::string& rejection_reason) const;

    // Internal execution result structure
    struct ExecutionResult {
        bool should_execute;
        bool is_partial_fill;
        double execution_price;
        double executed_quantity;
        std::chrono::milliseconds latency;
        std::string rejection_reason;
    };

    // Execution simulation
    std::vector<ExecutionResult> simulate_execution(std::shared_ptr<Order> order);

    // Price simulation
    double simulate_execution_price(std::shared_ptr<Order> order, double market_price) const;
    double simulate_slippage(std::shared_ptr<Order> order, double base_price) const;

    // Timing simulation
    std::chrono::milliseconds simulate_execution_latency() const;

    // Configuration
    void set_config(const SimulationConfig& config);
    const SimulationConfig& get_config() const;

    void set_market_data_provider(std::shared_ptr<IMarketDataProvider> provider);

    // Statistics
    struct ExecutionStats {
        size_t total_orders = 0;
        size_t executed_orders = 0;
        size_t rejected_orders = 0;
        size_t partial_fills = 0;
        double avg_latency_ms = 0.0;
        double avg_slippage_bps = 0.0;
        double fill_rate = 0.0;
    };

    ExecutionStats get_statistics() const;
    void reset_statistics();

private:
    // Configuration
    SimulationConfig config_;

    // Dependencies
    std::shared_ptr<IMarketDataProvider> market_data_provider_;

    // Random number generation
    mutable std::random_device rd_;
    mutable std::mt19937 gen_;
    mutable std::uniform_real_distribution<double> uniform_dist_;
    mutable std::normal_distribution<double> latency_dist_;
    mutable std::normal_distribution<double> slippage_dist_;

    // Statistics tracking
    mutable std::mutex stats_mutex_;
    mutable ExecutionStats stats_;

    // Helper methods
    double get_market_price(const std::string& symbol, OrderSide side) const;
    bool is_market_open() const;
    bool is_symbol_halted(const std::string& symbol) const;

    // Simulation logic
    bool should_partially_fill() const;
    double calculate_partial_fill_quantity(double total_quantity) const;
    double calculate_market_impact(std::shared_ptr<Order> order, double base_price) const;

    // Statistics updates
    void update_execution_stats(const ExecutionResult& result) const;

    // Logging
    void log_execution_event(const std::string& event, std::shared_ptr<Order> order) const;
};

/**
 * Market Condition Simulator
 * Simulates various market conditions for testing
 */
class MarketConditionSimulator {
public:
    enum class MarketCondition {
        NORMAL,         // Normal market conditions
        VOLATILE,       // High volatility
        ILLIQUID,       // Low liquidity
        TRENDING_UP,    // Strong upward trend
        TRENDING_DOWN,  // Strong downward trend
        GAPPING,        // Price gaps
        HALTED          // Market halted
    };

    explicit MarketConditionSimulator();

    // Market condition management
    void set_market_condition(MarketCondition condition);
    MarketCondition get_current_condition() const;

    // Condition effects on execution
    double get_liquidity_multiplier() const;
    double get_volatility_multiplier() const;
    double get_slippage_multiplier() const;
    double get_rejection_rate_multiplier() const;

    // Dynamic condition changes
    void enable_random_condition_changes(bool enable);
    void update_market_conditions();  // Call periodically to change conditions

private:
    MarketCondition current_condition_;
    bool random_changes_enabled_;

    mutable std::random_device rd_;
    mutable std::mt19937 gen_;
    mutable std::uniform_real_distribution<double> uniform_dist_;

    std::chrono::system_clock::time_point last_condition_change_;
    std::chrono::minutes condition_change_interval_{5}; // Change conditions every 5 minutes

    // Helper methods
    MarketCondition generate_random_condition() const;
};

/**
 * Execution Replay System
 * Records and replays execution scenarios for testing
 */
class ExecutionReplaySystem {
public:
    struct ExecutionRecord {
        std::string order_id;
        std::chrono::system_clock::time_point timestamp;
        OrderSide side;
        OrderType type;
        double quantity;
        double price;
        double execution_price;
        double executed_quantity;
        std::chrono::milliseconds latency;
        bool was_rejected;
        std::string rejection_reason;
    };

    ExecutionReplaySystem();

    // Recording
    void start_recording();
    void stop_recording();
    void record_execution(const ExecutionRecord& record);

    // Replay
    bool load_recording(const std::string& filename);
    bool save_recording(const std::string& filename) const;

    void start_replay();
    void stop_replay();
    bool get_next_execution(ExecutionRecord& record);

    // Analysis
    std::vector<ExecutionRecord> get_all_records() const;
    ExecutionSimulator::ExecutionStats analyze_recording() const;

    // Configuration
    void set_replay_speed(double speed_multiplier); // 1.0 = real-time, 2.0 = 2x speed

private:
    std::vector<ExecutionRecord> recorded_executions_;
    size_t replay_index_;
    double replay_speed_;
    bool is_recording_;
    bool is_replaying_;

    mutable std::mutex replay_mutex_;
    std::chrono::system_clock::time_point replay_start_time_;
    std::chrono::system_clock::time_point recording_start_time_;
};

/**
 * Execution Benchmark
 * Performance testing and validation for execution logic
 */
class ExecutionBenchmark {
public:
    struct BenchmarkConfig {
        size_t num_orders = 1000;
        double orders_per_second = 100.0;
        std::vector<std::string> symbols = {"AAPL", "GOOGL", "MSFT"};
        double min_quantity = 100.0;
        double max_quantity = 1000.0;
        double market_order_ratio = 0.7; // 70% market orders, 30% limit orders
    };

    explicit ExecutionBenchmark(std::shared_ptr<ExecutionSimulator> simulator);

    // Benchmark execution
    void run_benchmark(const BenchmarkConfig& config);

    // Results
    struct BenchmarkResults {
        std::chrono::milliseconds total_duration;
        double avg_orders_per_second;
        double avg_execution_latency_ms;
        double fill_rate;
        size_t total_orders;
        size_t successful_executions;
        size_t rejected_orders;
    };

    BenchmarkResults get_last_results() const;

    // Validation
    bool validate_execution_times() const;
    bool validate_fill_rates() const;

private:
    std::shared_ptr<ExecutionSimulator> simulator_;
    BenchmarkResults last_results_;

    // Helper methods
    std::shared_ptr<Order> generate_random_order(const BenchmarkConfig& config) const;
    void log_benchmark_results(const BenchmarkResults& results) const;

    mutable std::random_device rd_;
    mutable std::mt19937 gen_;
};

} // namespace trading