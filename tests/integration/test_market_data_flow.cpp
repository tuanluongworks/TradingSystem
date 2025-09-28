#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>

#include "infrastructure/market_data/market_data_provider.hpp"
#include "infrastructure/market_data/websocket_connector.hpp"
#include "core/models/market_tick.hpp"
#include "core/models/instrument.hpp"
#include "core/messaging/message_queue.hpp"
#include "utils/config.hpp"

using namespace trading;

class MarketDataFlowTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test configuration
        config_.mode = MarketDataProvider::ProviderMode::SIMULATION;
        config_.update_interval_ms = 50; // Fast updates for testing
        config_.default_symbols = {"AAPL", "GOOGL", "MSFT"};

        // Initialize message queue for market data
        market_data_queue_ = std::make_shared<MessageQueue<MarketTick>>(1000);

        // Initialize market data provider
        market_data_provider_ = std::make_shared<MarketDataProvider>(config_);

        // Set up market data callback
        received_ticks_.clear();
        market_data_provider_->set_tick_callback([this](const MarketTick& tick) {
            received_ticks_.push_back(tick);
            tick_count_.fetch_add(1);
        });

        // Set up connection callback
        connection_events_.clear();
        market_data_provider_->set_connection_callback([this](bool connected) {
            connection_events_.push_back(connected);
        });

        tick_count_ = 0;
    }

    void TearDown() override {
        if (market_data_provider_) {
            market_data_provider_->disconnect();
        }
    }

    MarketDataProvider::ProviderConfig config_;
    std::shared_ptr<MessageQueue<MarketTick>> market_data_queue_;
    std::shared_ptr<MarketDataProvider> market_data_provider_;
    std::vector<MarketTick> received_ticks_;
    std::vector<bool> connection_events_;
    std::atomic<int> tick_count_;
};

TEST_F(MarketDataFlowTest, ConnectionLifecycle) {
    // Act - Connect to market data
    bool connect_result = market_data_provider_->connect();

    // Assert
    EXPECT_TRUE(connect_result);
    EXPECT_TRUE(market_data_provider_->is_connected());

    // Wait for connection event
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify connection callback was triggered
    EXPECT_FALSE(connection_events_.empty());
    EXPECT_TRUE(connection_events_.back()); // Last event should be connection

    // Test disconnection
    market_data_provider_->disconnect();
    EXPECT_FALSE(market_data_provider_->is_connected());

    // Wait for disconnection event
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify disconnection callback was triggered
    if (connection_events_.size() > 1) {
        EXPECT_FALSE(connection_events_.back()); // Last event should be disconnection
    }
}

TEST_F(MarketDataFlowTest, SymbolSubscription) {
    // Arrange
    std::string test_symbol = "AAPL";

    // Connect first
    ASSERT_TRUE(market_data_provider_->connect());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Act - Subscribe to symbol
    bool subscribe_result = market_data_provider_->subscribe(test_symbol);

    // Assert
    EXPECT_TRUE(subscribe_result);

    // Verify symbol is in subscribed list
    auto subscribed_symbols = market_data_provider_->get_subscribed_symbols();
    EXPECT_FALSE(subscribed_symbols.empty());
    EXPECT_TRUE(std::find(subscribed_symbols.begin(), subscribed_symbols.end(), test_symbol)
                != subscribed_symbols.end());

    // Test unsubscription
    bool unsubscribe_result = market_data_provider_->unsubscribe(test_symbol);
    EXPECT_TRUE(unsubscribe_result);

    // Verify symbol is removed from subscribed list
    subscribed_symbols = market_data_provider_->get_subscribed_symbols();
    EXPECT_TRUE(std::find(subscribed_symbols.begin(), subscribed_symbols.end(), test_symbol)
                == subscribed_symbols.end());
}

TEST_F(MarketDataFlowTest, MarketDataReception) {
    // Arrange
    std::string test_symbol = "AAPL";

    // Connect and subscribe
    ASSERT_TRUE(market_data_provider_->connect());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ASSERT_TRUE(market_data_provider_->subscribe(test_symbol));

    // Act - Wait for market data
    auto start_time = std::chrono::steady_clock::now();
    auto timeout = std::chrono::seconds(5);

    while (received_ticks_.empty() &&
           (std::chrono::steady_clock::now() - start_time) < timeout) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Assert
    EXPECT_FALSE(received_ticks_.empty()) << "No market data received within timeout";

    if (!received_ticks_.empty()) {
        const auto& tick = received_ticks_[0];
        EXPECT_EQ(tick.instrument_symbol, test_symbol);
        EXPECT_GT(tick.bid_price, 0.0);
        EXPECT_GT(tick.ask_price, 0.0);
        EXPECT_GE(tick.ask_price, tick.bid_price); // Ask >= Bid
        EXPECT_GT(tick.last_price, 0.0);
        EXPECT_TRUE(tick.is_valid());
    }
}

TEST_F(MarketDataFlowTest, MultipleSymbolUpdates) {
    // Arrange
    std::vector<std::string> test_symbols = {"AAPL", "GOOGL", "MSFT"};

    // Connect first
    ASSERT_TRUE(market_data_provider_->connect());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Subscribe to multiple symbols
    for (const auto& symbol : test_symbols) {
        ASSERT_TRUE(market_data_provider_->subscribe(symbol));
    }

    // Act - Wait for market data from all symbols
    auto start_time = std::chrono::steady_clock::now();
    auto timeout = std::chrono::seconds(5);

    std::set<std::string> received_symbols;
    while (received_symbols.size() < test_symbols.size() &&
           (std::chrono::steady_clock::now() - start_time) < timeout) {

        for (const auto& tick : received_ticks_) {
            received_symbols.insert(tick.instrument_symbol);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Assert
    EXPECT_EQ(received_symbols.size(), test_symbols.size())
        << "Not all symbols received market data";

    // Verify each symbol received valid data
    for (const auto& symbol : test_symbols) {
        bool found_valid_tick = false;
        for (const auto& tick : received_ticks_) {
            if (tick.instrument_symbol == symbol && tick.is_valid()) {
                found_valid_tick = true;
                break;
            }
        }
        EXPECT_TRUE(found_valid_tick) << "No valid tick for symbol: " << symbol;
    }
}

TEST_F(MarketDataFlowTest, LatestTickRetrieval) {
    // Arrange
    std::string test_symbol = "AAPL";

    // Connect and subscribe
    ASSERT_TRUE(market_data_provider_->connect());
    ASSERT_TRUE(market_data_provider_->subscribe(test_symbol));

    // Wait for some ticks
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Act - Get latest tick
    auto latest_tick = market_data_provider_->get_latest_tick(test_symbol);

    // Assert
    ASSERT_NE(latest_tick, nullptr);
    EXPECT_EQ(latest_tick->instrument_symbol, test_symbol);
    EXPECT_TRUE(latest_tick->is_valid());

    // Test non-existent symbol
    auto non_existent_tick = market_data_provider_->get_latest_tick("NONEXISTENT");
    EXPECT_EQ(non_existent_tick, nullptr);
}

TEST_F(MarketDataFlowTest, RecentTicksHistory) {
    // Arrange
    std::string test_symbol = "AAPL";

    // Connect and subscribe
    ASSERT_TRUE(market_data_provider_->connect());
    ASSERT_TRUE(market_data_provider_->subscribe(test_symbol));

    // Wait for multiple ticks
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Act - Get recent ticks
    auto recent_ticks = market_data_provider_->get_recent_ticks(test_symbol, 5);

    // Assert
    EXPECT_FALSE(recent_ticks.empty());
    EXPECT_LE(recent_ticks.size(), 5); // Should not exceed requested count

    // Verify ticks are in chronological order (newest first)
    for (size_t i = 1; i < recent_ticks.size(); ++i) {
        EXPECT_GE(recent_ticks[i-1]->timestamp, recent_ticks[i]->timestamp)
            << "Ticks not in chronological order";
    }

    // Verify all ticks are for the correct symbol
    for (const auto& tick : recent_ticks) {
        EXPECT_EQ(tick->instrument_symbol, test_symbol);
        EXPECT_TRUE(tick->is_valid());
    }
}

TEST_F(MarketDataFlowTest, HighFrequencyUpdates) {
    // Arrange
    std::string test_symbol = "AAPL";
    config_.update_interval_ms = 10; // Very fast updates

    // Connect and subscribe
    ASSERT_TRUE(market_data_provider_->connect());
    ASSERT_TRUE(market_data_provider_->subscribe(test_symbol));

    // Act - Count ticks over a short period
    int initial_count = tick_count_.load();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 1 second
    int final_count = tick_count_.load();

    // Assert
    int ticks_received = final_count - initial_count;
    EXPECT_GT(ticks_received, 10) << "Expected high-frequency updates";

    // Verify no duplicate timestamps (within reason)
    std::set<std::chrono::system_clock::time_point> tick_timestamps;
    for (const auto& tick : received_ticks_) {
        tick_timestamps.insert(tick.timestamp);
    }

    // Allow some duplicates due to high frequency, but not too many
    double uniqueness_ratio = static_cast<double>(tick_timestamps.size()) / received_ticks_.size();
    EXPECT_GT(uniqueness_ratio, 0.5) << "Too many duplicate timestamps";
}

TEST_F(MarketDataFlowTest, PriceValidation) {
    // Arrange
    std::string test_symbol = "AAPL";

    // Connect and subscribe
    ASSERT_TRUE(market_data_provider_->connect());
    ASSERT_TRUE(market_data_provider_->subscribe(test_symbol));

    // Wait for market data
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Assert - Validate price relationships and ranges
    ASSERT_FALSE(received_ticks_.empty());

    for (const auto& tick : received_ticks_) {
        if (tick.instrument_symbol == test_symbol) {
            // Basic price validations
            EXPECT_GT(tick.bid_price, 0.0) << "Bid price should be positive";
            EXPECT_GT(tick.ask_price, 0.0) << "Ask price should be positive";
            EXPECT_GT(tick.last_price, 0.0) << "Last price should be positive";

            // Spread validation
            EXPECT_GE(tick.ask_price, tick.bid_price) << "Ask should be >= Bid";
            double spread = tick.get_spread();
            EXPECT_GE(spread, 0.0) << "Spread should be non-negative";
            EXPECT_LT(spread, tick.bid_price * 0.1) << "Spread too wide (>10% of bid)";

            // Price reasonableness (assuming AAPL is between $50-$500)
            EXPECT_GE(tick.last_price, 50.0) << "Price unreasonably low";
            EXPECT_LE(tick.last_price, 500.0) << "Price unreasonably high";

            // Mid price calculation
            double expected_mid = (tick.bid_price + tick.ask_price) / 2.0;
            EXPECT_DOUBLE_EQ(tick.get_mid_price(), expected_mid);
        }
    }
}

TEST_F(MarketDataFlowTest, ErrorHandling) {
    // Test subscription to invalid symbol
    std::string invalid_symbol = "INVALID_SYMBOL_123";

    ASSERT_TRUE(market_data_provider_->connect());

    // In simulation mode, subscription might succeed but no data should come
    bool subscribe_result = market_data_provider_->subscribe(invalid_symbol);
    (void)subscribe_result; // Suppress unused variable warning

    // Wait and verify no ticks for invalid symbol
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    bool found_invalid_tick = false;
    for (const auto& tick : received_ticks_) {
        if (tick.instrument_symbol == invalid_symbol) {
            found_invalid_tick = true;
            break;
        }
    }

    // In simulation mode, we might not get ticks for invalid symbols
    // The exact behavior depends on implementation
    // This test mainly ensures the system doesn't crash
    EXPECT_FALSE(found_invalid_tick) << "Received tick for invalid symbol";
}

