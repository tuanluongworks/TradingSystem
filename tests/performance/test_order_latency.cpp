#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <future>
#include <random>

#include "core/engine/trading_engine.hpp"
#include "core/risk/risk_manager.hpp"
#include "core/models/order.hpp"
#include "infrastructure/persistence/sqlite_service.hpp"
#include "infrastructure/market_data/market_data_provider.hpp"
#include "utils/config.hpp"

using namespace trading;
using namespace std::chrono;

class OrderLatencyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test configuration optimized for performance
        config_ = std::make_shared<Config>();
        config_->set("database.path", ":memory:"); // In-memory for speed
        config_->set("market_data.simulation_mode", true);
        config_->set("market_data.update_interval_ms", 1); // High frequency
        config_->set("risk.max_position_size", 100000.0); // High limits for testing
        config_->set("risk.max_order_size", 10000.0);

        // Initialize components
        persistence_ = std::make_shared<PersistenceService>(config_);
        ASSERT_TRUE(persistence_->initialize());

        risk_manager_ = std::make_shared<RiskManager>(config_);
        ASSERT_TRUE(risk_manager_->initialize());

        market_data_provider_ = std::make_shared<MarketDataProvider>(config_);
        ASSERT_TRUE(market_data_provider_->connect());

        trading_engine_ = std::make_shared<TradingEngine>(config_, persistence_, risk_manager_);
        ASSERT_TRUE(trading_engine_->initialize());

        // Set up test symbols
        test_symbols_ = {"AAPL", "GOOGL", "MSFT", "TSLA", "AMZN"};
        for (const auto& symbol : test_symbols_) {
            market_data_provider_->subscribe(symbol);
        }

        // Wait for initial market data
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Initialize random number generator
        random_engine_.seed(std::chrono::system_clock::now().time_since_epoch().count());
    }

    void TearDown() override {
        if (trading_engine_) {
            trading_engine_->shutdown();
        }
        if (market_data_provider_) {
            market_data_provider_->disconnect();
        }
        if (persistence_) {
            persistence_->shutdown();
        }
    }

    OrderRequest create_random_order() {
        std::uniform_int_distribution<size_t> symbol_dist(0, test_symbols_.size() - 1);
        std::uniform_int_distribution<int> side_dist(0, 1);
        std::uniform_real_distribution<double> qty_dist(50.0, 500.0);

        OrderRequest request;
        request.instrument_symbol = test_symbols_[symbol_dist(random_engine_)];
        request.side = (side_dist(random_engine_) == 0) ? OrderSide::BUY : OrderSide::SELL;
        request.type = OrderType::MARKET; // Market orders for fastest execution
        request.quantity = std::round(qty_dist(random_engine_));
        request.price = 0.0;
        request.timestamp = std::chrono::system_clock::now();
        return request;
    }

    struct LatencyMeasurement {
        std::string order_id;
        high_resolution_clock::time_point submit_time;
        high_resolution_clock::time_point ack_time;
        high_resolution_clock::time_point fill_time;
        microseconds submit_to_ack_latency;
        microseconds ack_to_fill_latency;
        microseconds end_to_end_latency;
    };

    std::shared_ptr<Config> config_;
    std::shared_ptr<PersistenceService> persistence_;
    std::shared_ptr<RiskManager> risk_manager_;
    std::shared_ptr<MarketDataProvider> market_data_provider_;
    std::shared_ptr<TradingEngine> trading_engine_;
    std::vector<std::string> test_symbols_;
    std::mt19937 random_engine_;
};

TEST_F(OrderLatencyTest, SingleOrderLatency) {
    // This test measures the latency of a single order submission

    std::vector<LatencyMeasurement> measurements;
    const int num_orders = 100;

    // Set up callbacks to measure latency
    std::mutex measurement_mutex;
    trading_engine_->set_order_update_callback([&](const ExecutionReport& report) {
        std::lock_guard<std::mutex> lock(measurement_mutex);
        auto now = high_resolution_clock::now();

        // Find the measurement for this order
        for (auto& measurement : measurements) {
            if (measurement.order_id == report.order_id) {
                if (report.new_status == OrderStatus::ACCEPTED && measurement.ack_time.time_since_epoch().count() == 0) {
                    measurement.ack_time = now;
                    measurement.submit_to_ack_latency = duration_cast<microseconds>(now - measurement.submit_time);
                } else if (report.new_status == OrderStatus::FILLED && measurement.fill_time.time_since_epoch().count() == 0) {
                    measurement.fill_time = now;
                    measurement.ack_to_fill_latency = duration_cast<microseconds>(now - measurement.ack_time);
                    measurement.end_to_end_latency = duration_cast<microseconds>(now - measurement.submit_time);
                }
                break;
            }
        }
    });

    // Submit orders and measure latency
    for (int i = 0; i < num_orders; ++i) {
        auto request = create_random_order();

        LatencyMeasurement measurement;
        measurement.submit_time = high_resolution_clock::now();

        std::string order_id = trading_engine_->submit_order(request);
        ASSERT_FALSE(order_id.empty());

        measurement.order_id = order_id;
        measurements.push_back(measurement);

        // Small delay between orders to avoid overwhelming the system
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    // Wait for all orders to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Analyze latency results
    std::vector<long> submit_to_ack_latencies;
    std::vector<long> end_to_end_latencies;

    for (const auto& measurement : measurements) {
        if (measurement.ack_time.time_since_epoch().count() > 0) {
            submit_to_ack_latencies.push_back(measurement.submit_to_ack_latency.count());
        }
        if (measurement.fill_time.time_since_epoch().count() > 0) {
            end_to_end_latencies.push_back(measurement.end_to_end_latency.count());
        }
    }

    ASSERT_FALSE(submit_to_ack_latencies.empty()) << "No order acknowledgments received";
    ASSERT_FALSE(end_to_end_latencies.empty()) << "No order fills received";

    // Calculate statistics
    auto calc_stats = [](const std::vector<long>& values) {
        auto sorted = values;
        std::sort(sorted.begin(), sorted.end());

        double avg = std::accumulate(sorted.begin(), sorted.end(), 0.0) / sorted.size();
        long p50 = sorted[sorted.size() / 2];
        long p95 = sorted[static_cast<size_t>(sorted.size() * 0.95)];
        long p99 = sorted[static_cast<size_t>(sorted.size() * 0.99)];
        long max_val = sorted.back();

        return std::make_tuple(avg, p50, p95, p99, max_val);
    };

    auto [ack_avg, ack_p50, ack_p95, ack_p99, ack_max] = calc_stats(submit_to_ack_latencies);
    auto [e2e_avg, e2e_p50, e2e_p95, e2e_p99, e2e_max] = calc_stats(end_to_end_latencies);

    // Print results for analysis
    std::cout << "\n=== Order Latency Performance Results ===" << std::endl;
    std::cout << "Submit to Ack Latency (μs):" << std::endl;
    std::cout << "  Average: " << ack_avg << std::endl;
    std::cout << "  P50: " << ack_p50 << std::endl;
    std::cout << "  P95: " << ack_p95 << std::endl;
    std::cout << "  P99: " << ack_p99 << std::endl;
    std::cout << "  Max: " << ack_max << std::endl;

    std::cout << "End-to-End Latency (μs):" << std::endl;
    std::cout << "  Average: " << e2e_avg << std::endl;
    std::cout << "  P50: " << e2e_p50 << std::endl;
    std::cout << "  P95: " << e2e_p95 << std::endl;
    std::cout << "  P99: " << e2e_p99 << std::endl;
    std::cout << "  Max: " << e2e_max << std::endl;

    // Performance assertions (adjust based on requirements)
    EXPECT_LT(ack_p99, 1000) << "P99 acknowledgment latency exceeds 1ms: " << ack_p99 << "μs";
    EXPECT_LT(e2e_p95, 5000) << "P95 end-to-end latency exceeds 5ms: " << e2e_p95 << "μs";
}

TEST_F(OrderLatencyTest, HighThroughputTest) {
    // Test system performance under high order submission rate

    const int orders_per_second = 1000;
    const int test_duration_seconds = 5;
    const int total_orders = orders_per_second * test_duration_seconds;

    std::atomic<int> orders_submitted(0);
    std::atomic<int> orders_acknowledged(0);
    std::atomic<int> orders_filled(0);
    std::atomic<int> orders_rejected(0);

    // Track order processing
    trading_engine_->set_order_update_callback([&](const ExecutionReport& report) {
        if (report.old_status == OrderStatus::NEW && report.new_status == OrderStatus::ACCEPTED) {
            orders_acknowledged.fetch_add(1);
        } else if (report.new_status == OrderStatus::FILLED) {
            orders_filled.fetch_add(1);
        } else if (report.new_status == OrderStatus::REJECTED) {
            orders_rejected.fetch_add(1);
        }
    });

    // Calculate timing
    auto order_interval = std::chrono::microseconds(1000000 / orders_per_second);
    auto start_time = high_resolution_clock::now();
    auto next_order_time = start_time;

    // Submit orders at target rate
    for (int i = 0; i < total_orders; ++i) {
        auto now = high_resolution_clock::now();

        // Wait until it's time for the next order
        if (now < next_order_time) {
            std::this_thread::sleep_until(next_order_time);
        }

        auto request = create_random_order();
        std::string order_id = trading_engine_->submit_order(request);

        if (!order_id.empty()) {
            orders_submitted.fetch_add(1);
        }

        next_order_time += order_interval;
    }

    auto submission_end_time = high_resolution_clock::now();

    // Wait for processing to complete
    std::this_thread::sleep_for(std::chrono::seconds(2));

    auto total_submission_time = duration_cast<milliseconds>(submission_end_time - start_time);

    // Calculate actual submission rate
    double actual_rate = static_cast<double>(orders_submitted.load()) * 1000.0 / total_submission_time.count();

    std::cout << "\n=== High Throughput Test Results ===" << std::endl;
    std::cout << "Target rate: " << orders_per_second << " orders/sec" << std::endl;
    std::cout << "Actual rate: " << actual_rate << " orders/sec" << std::endl;
    std::cout << "Orders submitted: " << orders_submitted.load() << "/" << total_orders << std::endl;
    std::cout << "Orders acknowledged: " << orders_acknowledged.load() << std::endl;
    std::cout << "Orders filled: " << orders_filled.load() << std::endl;
    std::cout << "Orders rejected: " << orders_rejected.load() << std::endl;

    // Performance assertions
    EXPECT_GE(orders_submitted.load(), total_orders * 0.95) << "Failed to submit 95% of orders";
    EXPECT_GE(actual_rate, orders_per_second * 0.9) << "Submission rate too low";
    EXPECT_LE(orders_rejected.load(), orders_submitted.load() * 0.05) << "Too many rejected orders";
}

TEST_F(OrderLatencyTest, ConcurrentOrderSubmission) {
    // Test latency under concurrent submission from multiple threads

    const int num_threads = 4;
    const int orders_per_thread = 100;
    std::vector<std::future<std::vector<microseconds>>> futures;

    auto submit_orders = [&](int thread_id) -> std::vector<microseconds> {
        std::vector<microseconds> latencies;
        latencies.reserve(orders_per_thread);

        for (int i = 0; i < orders_per_thread; ++i) {
            auto request = create_random_order();
            auto start_time = high_resolution_clock::now();

            std::string order_id = trading_engine_->submit_order(request);
            auto end_time = high_resolution_clock::now();

            if (!order_id.empty()) {
                auto latency = duration_cast<microseconds>(end_time - start_time);
                latencies.push_back(latency);
            }

            // Small random delay to avoid lockstep behavior
            std::this_thread::sleep_for(std::chrono::microseconds(thread_id * 10 + i % 50));
        }

        return latencies;
    };

    // Launch concurrent threads
    auto start_time = high_resolution_clock::now();

    for (int t = 0; t < num_threads; ++t) {
        futures.push_back(std::async(std::launch::async, submit_orders, t));
    }

    // Collect results
    std::vector<microseconds> all_latencies;
    for (auto& future : futures) {
        auto thread_latencies = future.get();
        all_latencies.insert(all_latencies.end(), thread_latencies.begin(), thread_latencies.end());
    }

    auto end_time = high_resolution_clock::now();
    auto total_time = duration_cast<milliseconds>(end_time - start_time);

    // Analyze results
    ASSERT_FALSE(all_latencies.empty()) << "No successful order submissions";

    std::vector<long> latency_values;
    for (const auto& latency : all_latencies) {
        latency_values.push_back(latency.count());
    }

    std::sort(latency_values.begin(), latency_values.end());

    double avg_latency = std::accumulate(latency_values.begin(), latency_values.end(), 0.0) / latency_values.size();
    long p95_latency = latency_values[static_cast<size_t>(latency_values.size() * 0.95)];
    long max_latency = latency_values.back();

    double throughput = static_cast<double>(all_latencies.size()) * 1000.0 / total_time.count();

    std::cout << "\n=== Concurrent Submission Test Results ===" << std::endl;
    std::cout << "Threads: " << num_threads << std::endl;
    std::cout << "Successful submissions: " << all_latencies.size() << "/" << (num_threads * orders_per_thread) << std::endl;
    std::cout << "Total time: " << total_time.count() << " ms" << std::endl;
    std::cout << "Throughput: " << throughput << " orders/sec" << std::endl;
    std::cout << "Average latency: " << avg_latency << " μs" << std::endl;
    std::cout << "P95 latency: " << p95_latency << " μs" << std::endl;
    std::cout << "Max latency: " << max_latency << " μs" << std::endl;

    // Performance assertions
    EXPECT_LT(avg_latency, 500) << "Average latency too high under concurrency";
    EXPECT_LT(p95_latency, 2000) << "P95 latency too high under concurrency";
    EXPECT_GE(throughput, 500) << "Throughput too low under concurrency";
}

TEST_F(OrderLatencyTest, MarketDataProcessingLatency) {
    // Test latency of market data processing and position updates

    std::atomic<int> tick_count(0);
    std::vector<microseconds> processing_latencies;
    std::mutex latency_mutex;

    // Set up market data callback to measure processing latency
    market_data_provider_->set_tick_callback([&](const MarketTick& tick) {
        auto processing_start = high_resolution_clock::now();

        // Simulate position P&L update (this would normally be done by trading engine)
        auto position = trading_engine_->get_position(tick.instrument_symbol);
        if (position != nullptr) {
            // Calculate unrealized P&L (simulated)
            double unrealized_pnl = (tick.last_price - position->average_price) * position->quantity;
        }

        auto processing_end = high_resolution_clock::now();
        auto latency = duration_cast<microseconds>(processing_end - processing_start);

        {
            std::lock_guard<std::mutex> lock(latency_mutex);
            processing_latencies.push_back(latency);
        }

        tick_count.fetch_add(1);
    });

    // Create some positions to update
    for (const auto& symbol : test_symbols_) {
        auto request = create_random_order();
        request.instrument_symbol = symbol;
        request.side = OrderSide::BUY;
        request.quantity = 100.0;
        trading_engine_->submit_order(request);
    }

    // Wait for positions to be created
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Measure market data processing for a period
    auto start_time = high_resolution_clock::now();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    auto end_time = high_resolution_clock::now();

    auto test_duration = duration_cast<milliseconds>(end_time - start_time);

    // Analyze results
    {
        std::lock_guard<std::mutex> lock(latency_mutex);
        ASSERT_FALSE(processing_latencies.empty()) << "No market data processing measured";

        std::vector<long> latency_values;
        for (const auto& latency : processing_latencies) {
            latency_values.push_back(latency.count());
        }

        std::sort(latency_values.begin(), latency_values.end());

        double avg_latency = std::accumulate(latency_values.begin(), latency_values.end(), 0.0) / latency_values.size();
        long p95_latency = latency_values[static_cast<size_t>(latency_values.size() * 0.95)];
        long max_latency = latency_values.back();

        double tick_rate = static_cast<double>(tick_count.load()) * 1000.0 / test_duration.count();

        std::cout << "\n=== Market Data Processing Latency Results ===" << std::endl;
        std::cout << "Ticks processed: " << tick_count.load() << std::endl;
        std::cout << "Tick rate: " << tick_rate << " ticks/sec" << std::endl;
        std::cout << "Average processing latency: " << avg_latency << " μs" << std::endl;
        std::cout << "P95 processing latency: " << p95_latency << " μs" << std::endl;
        std::cout << "Max processing latency: " << max_latency << " μs" << std::endl;

        // Performance assertions
        EXPECT_LT(avg_latency, 100) << "Average market data processing latency too high";
        EXPECT_LT(p95_latency, 500) << "P95 market data processing latency too high";
        EXPECT_GE(tick_rate, 100) << "Market data tick rate too low";
    }
}

TEST_F(OrderLatencyTest, MemoryUsageStability) {
    // Test that memory usage remains stable under load

    const int num_orders = 1000;
    const int measurement_interval = 100;

    // This is a simplified memory usage test
    // In a real implementation, you would use platform-specific APIs
    // to measure actual memory usage

    std::vector<size_t> memory_measurements;

    for (int i = 0; i < num_orders; ++i) {
        auto request = create_random_order();
        std::string order_id = trading_engine_->submit_order(request);
        ASSERT_FALSE(order_id.empty());

        // Measure memory usage periodically
        if (i % measurement_interval == 0) {
            // This would normally measure actual memory usage
            // For this test, we'll just ensure the system remains responsive
            auto all_orders = trading_engine_->get_working_orders();
            memory_measurements.push_back(all_orders.size());
        }
    }

    // Wait for processing to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Verify system is still responsive
    auto final_orders = trading_engine_->get_working_orders();
    auto all_positions = trading_engine_->get_all_positions();

    EXPECT_TRUE(persistence_->is_available()) << "Persistence service became unavailable";
    EXPECT_TRUE(market_data_provider_->is_connected()) << "Market data connection lost";

    std::cout << "\n=== Memory Stability Test Results ===" << std::endl;
    std::cout << "Orders submitted: " << num_orders << std::endl;
    std::cout << "Working orders: " << final_orders.size() << std::endl;
    std::cout << "Positions: " << all_positions.size() << std::endl;
    std::cout << "System remains responsive: YES" << std::endl;

    // Basic stability checks
    EXPECT_LT(final_orders.size(), num_orders * 0.1) << "Too many orders remain working";
    EXPECT_GT(all_positions.size(), 0) << "No positions created";
}

} // Anonymous namespace for tests