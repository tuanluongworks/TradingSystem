#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>

#include "core/engine/trading_engine.hpp"
#include "core/risk/risk_manager.hpp"
#include "core/models/position.hpp"
#include "core/models/order.hpp"
#include "core/models/trade.hpp"
#include "infrastructure/persistence/sqlite_service.hpp"
#include "infrastructure/market_data/market_data_provider.hpp"
#include "utils/config.hpp"

using namespace trading;

class PositionTrackingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test configuration
        config_ = std::make_shared<TradingSystemConfig>();
        config_->persistence.database_path = ":memory:"; // In-memory database for testing
        config_->market_data.simulation_mode = true;

        // Initialize persistence service
        persistence_ = std::make_shared<SQLiteService>(config_->persistence.database_path);
        ASSERT_TRUE(persistence_->initialize());

        // Initialize market data provider
        MarketDataProvider::ProviderConfig md_config;
        md_config.mode = MarketDataProvider::ProviderMode::SIMULATION;
        market_data_provider_ = std::make_shared<MarketDataProvider>(md_config);
        ASSERT_TRUE(market_data_provider_->connect());

        // Initialize trading engine
        auto risk_manager = std::make_shared<RiskManager>(config_->risk_management);
        trading_engine_ = std::make_shared<TradingEngine>(risk_manager, persistence_);
        ASSERT_TRUE(trading_engine_->initialize());

        // Set up test symbols
        test_symbols_ = {"AAPL", "GOOGL", "MSFT"};
        setup_market_data();

        // Track position updates
        position_updates_.clear();
        trading_engine_->set_position_update_callback([this](const Position& position) {
            position_updates_.push_back(std::make_shared<Position>(position.get_instrument_symbol()));
        });
    }

    void TearDown() override {
        if (trading_engine_) {
            trading_engine_->shutdown();
        }
        if (market_data_provider_) {
            market_data_provider_->disconnect();
        }
        if (persistence_) {
            persistence_->close();
        }
    }

    void setup_market_data() {
        // Subscribe to test symbols
        for (const auto& symbol : test_symbols_) {
            market_data_provider_->subscribe(symbol);
        }

        // Wait for initial market data
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    OrderRequest create_order_request(const std::string& symbol, OrderSide side,
                                    OrderType type, double quantity, double price = 0.0) {
        OrderRequest request;
        request.instrument_symbol = symbol;
        request.side = side;
        request.type = type;
        request.quantity = quantity;
        request.price = price;
        request.timestamp = std::chrono::system_clock::now();
        return request;
    }

    std::shared_ptr<TradingSystemConfig> config_;
    std::shared_ptr<SQLiteService> persistence_;
    std::shared_ptr<MarketDataProvider> market_data_provider_;
    std::shared_ptr<TradingEngine> trading_engine_;
    std::vector<std::string> test_symbols_;
    std::vector<std::shared_ptr<Position>> position_updates_;
};

TEST_F(PositionTrackingTest, InitialPositionCreation) {
    // Arrange
    std::string symbol = test_symbols_[0];
    double quantity = 100.0;

    // Verify no initial position
    auto initial_position = trading_engine_->get_position(symbol);
    EXPECT_EQ(initial_position, nullptr);

    // Act - Execute a buy order
    auto buy_request = create_order_request(symbol, OrderSide::BUY, OrderType::MARKET, quantity);
    std::string order_id = trading_engine_->submit_order(buy_request);
    ASSERT_FALSE(order_id.empty());

    // Wait for execution
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Assert - Verify position was created
    auto position = trading_engine_->get_position(symbol);
    ASSERT_NE(position, nullptr);
    EXPECT_EQ(position->get_instrument_symbol(), symbol);
    EXPECT_EQ(position->get_quantity(), quantity);
    EXPECT_GT(position->get_average_price(), 0.0);
    EXPECT_EQ(position->get_realized_pnl(), 0.0); // No realized P&L yet

    // Verify position update callback was triggered
    EXPECT_FALSE(position_updates_.empty());
    bool found_position_update = false;
    for (const auto& update : position_updates_) {
        if (update->get_instrument_symbol() == symbol && update->get_quantity() == quantity) {
            found_position_update = true;
            break;
        }
    }
    EXPECT_TRUE(found_position_update);
}

TEST_F(PositionTrackingTest, LongPositionIncrease) {
    // Arrange - Create initial position
    std::string symbol = test_symbols_[0];
    auto buy_request1 = create_order_request(symbol, OrderSide::BUY, OrderType::MARKET, 100.0);
    std::string order_id1 = trading_engine_->submit_order(buy_request1);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto initial_position = trading_engine_->get_position(symbol);
    ASSERT_NE(initial_position, nullptr);
    double initial_quantity = initial_position->get_quantity();
    double initial_avg_price = initial_position->get_average_price();
    (void)initial_avg_price; // Suppress unused variable warning

    // Act - Add to position
    auto buy_request2 = create_order_request(symbol, OrderSide::BUY, OrderType::MARKET, 50.0);
    std::string order_id2 = trading_engine_->submit_order(buy_request2);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Assert - Verify position increased
    auto final_position = trading_engine_->get_position(symbol);
    ASSERT_NE(final_position, nullptr);
    EXPECT_EQ(final_position->get_quantity(), initial_quantity + 50.0);

    // Verify average price calculation
    // New average should be weighted by quantities
    EXPECT_GT(final_position->get_average_price(), 0.0);
    // Exact average price calculation depends on execution prices
}

TEST_F(PositionTrackingTest, PartialPositionReduce) {
    // Arrange - Create initial long position
    std::string symbol = test_symbols_[0];
    auto buy_request = create_order_request(symbol, OrderSide::BUY, OrderType::MARKET, 200.0);
    std::string buy_order_id = trading_engine_->submit_order(buy_request);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto initial_position = trading_engine_->get_position(symbol);
    ASSERT_NE(initial_position, nullptr);
    EXPECT_EQ(initial_position->get_quantity(), 200.0);

    // Act - Partially sell position
    auto sell_request = create_order_request(symbol, OrderSide::SELL, OrderType::MARKET, 75.0);
    std::string sell_order_id = trading_engine_->submit_order(sell_request);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Assert - Verify position reduced
    auto final_position = trading_engine_->get_position(symbol);
    ASSERT_NE(final_position, nullptr);
    EXPECT_EQ(final_position->get_quantity(), 125.0); // 200 - 75
    EXPECT_EQ(final_position->get_average_price(), initial_position->get_average_price()); // Avg price unchanged
    EXPECT_NE(final_position->get_realized_pnl(), 0.0); // Should have realized P&L
}

TEST_F(PositionTrackingTest, PositionFlattening) {
    // Arrange - Create long position
    std::string symbol = test_symbols_[0];
    auto buy_request = create_order_request(symbol, OrderSide::BUY, OrderType::MARKET, 100.0);
    std::string buy_order_id = trading_engine_->submit_order(buy_request);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto initial_position = trading_engine_->get_position(symbol);
    ASSERT_NE(initial_position, nullptr);
    double initial_quantity = initial_position->get_quantity();

    // Act - Sell entire position
    auto sell_request = create_order_request(symbol, OrderSide::SELL, OrderType::MARKET, initial_quantity);
    std::string sell_order_id = trading_engine_->submit_order(sell_request);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Assert - Verify position is flat
    auto final_position = trading_engine_->get_position(symbol);
    if (final_position != nullptr) {
        EXPECT_TRUE(final_position->is_flat());
        EXPECT_EQ(final_position->get_quantity(), 0.0);
    }
    // Note: Some implementations might delete flat positions entirely
}

TEST_F(PositionTrackingTest, ShortPositionCreation) {
    // Note: This test assumes the system supports short selling
    // If short selling is not allowed, this test might fail as expected

    // Arrange
    std::string symbol = test_symbols_[0];
    double quantity = 100.0;

    // Act - Try to sell without existing position (short sell)
    auto sell_request = create_order_request(symbol, OrderSide::SELL, OrderType::MARKET, quantity);
    std::string order_id = trading_engine_->submit_order(sell_request);

    if (!order_id.empty()) {
        // Wait for execution
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // Assert - Verify short position was created
        auto position = trading_engine_->get_position(symbol);
        ASSERT_NE(position, nullptr);
        EXPECT_EQ(position->get_quantity(), -quantity); // Negative for short
        EXPECT_GT(position->get_average_price(), 0.0);
    } else {
        // Short selling not allowed - verify order was rejected
        EXPECT_TRUE(order_id.empty());
        auto position = trading_engine_->get_position(symbol);
        EXPECT_EQ(position, nullptr);
    }
}

TEST_F(PositionTrackingTest, UnrealizedPnLCalculation) {
    // Arrange - Create position
    std::string symbol = test_symbols_[0];
    auto buy_request = create_order_request(symbol, OrderSide::BUY, OrderType::MARKET, 100.0);
    std::string order_id = trading_engine_->submit_order(buy_request);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto position = trading_engine_->get_position(symbol);
    ASSERT_NE(position, nullptr);
    double initial_avg_price = position->get_average_price();

    // Act - Wait for potential price changes
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Get current market price
    auto latest_tick = market_data_provider_->get_latest_tick(symbol);
    ASSERT_NE(latest_tick, nullptr);
    double current_price = latest_tick->last_price;

    // Refresh position (this might trigger P&L recalculation)
    position = trading_engine_->get_position(symbol);
    ASSERT_NE(position, nullptr);

    // Assert - Verify unrealized P&L calculation
    double expected_unrealized_pnl = (current_price - initial_avg_price) * position->get_quantity();
    EXPECT_DOUBLE_EQ(position->get_unrealized_pnl(), expected_unrealized_pnl);

    // Verify total P&L
    double expected_total_pnl = position->get_realized_pnl() + position->get_unrealized_pnl();
    EXPECT_DOUBLE_EQ(position->get_total_pnl(current_price), expected_total_pnl);
}

TEST_F(PositionTrackingTest, MultipleSymbolPositions) {
    // Arrange
    std::vector<double> quantities = {100.0, 150.0, 200.0};

    // Act - Create positions in multiple symbols
    for (size_t i = 0; i < test_symbols_.size(); ++i) {
        auto buy_request = create_order_request(test_symbols_[i], OrderSide::BUY,
                                              OrderType::MARKET, quantities[i]);
        std::string order_id = trading_engine_->submit_order(buy_request);
        ASSERT_FALSE(order_id.empty());
    }

    // Wait for all executions
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Assert - Verify all positions were created
    auto all_positions = trading_engine_->get_all_positions();
    EXPECT_EQ(all_positions.size(), test_symbols_.size());

    for (size_t i = 0; i < test_symbols_.size(); ++i) {
        auto position = trading_engine_->get_position(test_symbols_[i]);
        ASSERT_NE(position, nullptr);
        EXPECT_EQ(position->get_instrument_symbol(), test_symbols_[i]);
        EXPECT_EQ(position->get_quantity(), quantities[i]);
        EXPECT_GT(position->get_average_price(), 0.0);
    }
}

TEST_F(PositionTrackingTest, RealizedPnLAccumulation) {
    // Arrange - Create position and then trade it multiple times
    std::string symbol = test_symbols_[0];

    // Initial buy
    auto buy_request1 = create_order_request(symbol, OrderSide::BUY, OrderType::MARKET, 100.0);
    trading_engine_->submit_order(buy_request1);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Partial sell
    auto sell_request1 = create_order_request(symbol, OrderSide::SELL, OrderType::MARKET, 50.0);
    trading_engine_->submit_order(sell_request1);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto position_after_sell1 = trading_engine_->get_position(symbol);
    ASSERT_NE(position_after_sell1, nullptr);
    double realized_pnl_1 = position_after_sell1->get_realized_pnl();

    // Another buy
    auto buy_request2 = create_order_request(symbol, OrderSide::BUY, OrderType::MARKET, 75.0);
    trading_engine_->submit_order(buy_request2);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Another partial sell
    auto sell_request2 = create_order_request(symbol, OrderSide::SELL, OrderType::MARKET, 25.0);
    trading_engine_->submit_order(sell_request2);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Assert - Verify realized P&L accumulated
    auto final_position = trading_engine_->get_position(symbol);
    ASSERT_NE(final_position, nullptr);

    // Realized P&L should have increased (or decreased based on prices)
    // The exact value depends on execution prices, but it should be different
    EXPECT_NE(final_position->get_realized_pnl(), realized_pnl_1);

    // Verify position quantity is correct
    double expected_quantity = 100.0 - 50.0 + 75.0 - 25.0; // 100
    EXPECT_EQ(final_position->get_quantity(), expected_quantity);
}

TEST_F(PositionTrackingTest, PositionPersistence) {
    // Arrange - Create position
    std::string symbol = test_symbols_[0];
    auto buy_request = create_order_request(symbol, OrderSide::BUY, OrderType::MARKET, 100.0);
    std::string order_id = trading_engine_->submit_order(buy_request);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto original_position = trading_engine_->get_position(symbol);
    ASSERT_NE(original_position, nullptr);

    // Act - Simulate system restart by creating new engine instance
    trading_engine_->shutdown();
    auto risk_manager = std::make_shared<RiskManager>(config_->risk_management);
    trading_engine_ = std::make_shared<TradingEngine>(risk_manager, persistence_);
    ASSERT_TRUE(trading_engine_->initialize());

    // Assert - Verify position was restored
    auto restored_position = trading_engine_->get_position(symbol);
    ASSERT_NE(restored_position, nullptr);
    EXPECT_EQ(restored_position->get_instrument_symbol(), original_position->get_instrument_symbol());
    EXPECT_EQ(restored_position->get_quantity(), original_position->get_quantity());
    EXPECT_EQ(restored_position->get_average_price(), original_position->get_average_price());
    EXPECT_EQ(restored_position->get_realized_pnl(), original_position->get_realized_pnl());
}

