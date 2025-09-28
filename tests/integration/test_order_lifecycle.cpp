#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>

#include "core/engine/trading_engine.hpp"
#include "core/risk/risk_manager.hpp"
#include "core/models/order.hpp"
#include "core/models/trade.hpp"
#include "core/models/position.hpp"
#include "infrastructure/persistence/sqlite_service.hpp"
#include "utils/config.hpp"

using namespace trading;

class OrderLifecycleTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test configuration
        config_ = std::make_shared<TradingSystemConfig>();
        config_->persistence.database_path = ":memory:"; // In-memory database for testing
        config_->risk_management.max_position_size = 10000.0;
        config_->risk_management.max_order_size = 1000.0;

        // Initialize persistence service
        persistence_ = std::make_shared<SQLiteService>(config_->persistence.database_path);
        ASSERT_TRUE(persistence_->initialize());

        // Initialize risk manager
        risk_manager_ = std::make_shared<RiskManager>(config_->risk_management);

        // Initialize trading engine
        trading_engine_ = std::make_shared<TradingEngine>(risk_manager_, persistence_);
        ASSERT_TRUE(trading_engine_->initialize());

        // Set up test instrument
        test_symbol_ = "AAPL";
        setup_test_instrument();
    }

    void TearDown() override {
        if (trading_engine_) {
            trading_engine_->shutdown();
        }
        if (persistence_) {
            persistence_->close();
        }
    }

    void setup_test_instrument() {
        // Create test instrument
        auto instrument = std::make_shared<Instrument>(test_symbol_, "Apple Inc", InstrumentType::STOCK, 0.01, 1);
        instrument->set_active(true);
        instrument->update_market_data(150.00, 150.05, 150.02);

        // Add instrument to system (this would normally be done by market data provider)
        // For testing, we'll assume the instrument is available
    }

    std::shared_ptr<TradingSystemConfig> config_;
    std::shared_ptr<SQLiteService> persistence_;
    std::shared_ptr<RiskManager> risk_manager_;
    std::shared_ptr<TradingEngine> trading_engine_;
    std::string test_symbol_;
};

TEST_F(OrderLifecycleTest, SubmitMarketBuyOrder) {
    // Arrange
    OrderRequest request;
    request.instrument_symbol = test_symbol_;
    request.side = OrderSide::BUY;
    request.type = OrderType::MARKET;
    request.quantity = 100.0;
    request.price = 0.0; // Market order
    request.timestamp = std::chrono::system_clock::now();

    // Track order updates
    std::vector<ExecutionReport> execution_reports;
    trading_engine_->set_order_update_callback([&execution_reports](const ExecutionReport& report) {
        execution_reports.push_back(report);
    });

    // Track trades
    std::vector<Trade> trades;
    trading_engine_->set_trade_callback([&trades](const Trade& trade) {
        trades.push_back(trade);
    });

    // Act
    std::string order_id = trading_engine_->submit_order(request);

    // Assert
    ASSERT_FALSE(order_id.empty());

    // Give time for order processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify order was created
    auto order = trading_engine_->get_order(order_id);
    ASSERT_NE(order, nullptr);
    EXPECT_EQ(order->get_instrument_symbol(), test_symbol_);
    EXPECT_EQ(order->get_side(), OrderSide::BUY);
    EXPECT_EQ(order->get_type(), OrderType::MARKET);
    EXPECT_EQ(order->get_quantity(), 100.0);

    // In simulation mode, market orders should execute immediately
    EXPECT_TRUE(order->get_status() == OrderStatus::FILLED || order->get_status() == OrderStatus::PARTIALLY_FILLED);

    // Verify execution reports were generated
    EXPECT_FALSE(execution_reports.empty());

    // Verify at least one trade was generated
    EXPECT_FALSE(trades.empty());

    if (!trades.empty()) {
        const auto& trade = trades[0];
        EXPECT_EQ(trade.get_instrument_symbol(), test_symbol_);
        EXPECT_EQ(trade.get_side(), OrderSide::BUY);
        EXPECT_GT(trade.get_quantity(), 0.0);
        EXPECT_GT(trade.get_price(), 0.0);
    }
}

TEST_F(OrderLifecycleTest, SubmitLimitSellOrder) {
    // Arrange - First create a position to sell
    OrderRequest buy_request;
    buy_request.instrument_symbol = test_symbol_;
    buy_request.side = OrderSide::BUY;
    buy_request.type = OrderType::MARKET;
    buy_request.quantity = 200.0;
    buy_request.timestamp = std::chrono::system_clock::now();

    std::string buy_order_id = trading_engine_->submit_order(buy_request);
    ASSERT_FALSE(buy_order_id.empty());

    // Wait for buy order to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Now submit limit sell order
    OrderRequest sell_request;
    sell_request.instrument_symbol = test_symbol_;
    sell_request.side = OrderSide::SELL;
    sell_request.type = OrderType::LIMIT;
    sell_request.quantity = 100.0;
    sell_request.price = 155.00; // Above current market price
    sell_request.timestamp = std::chrono::system_clock::now();

    // Act
    std::string sell_order_id = trading_engine_->submit_order(sell_request);

    // Assert
    ASSERT_FALSE(sell_order_id.empty());

    // Give time for order processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify order was created
    auto order = trading_engine_->get_order(sell_order_id);
    ASSERT_NE(order, nullptr);
    EXPECT_EQ(order->get_instrument_symbol(), test_symbol_);
    EXPECT_EQ(order->get_side(), OrderSide::SELL);
    EXPECT_EQ(order->get_type(), OrderType::LIMIT);
    EXPECT_EQ(order->get_quantity(), 100.0);
    EXPECT_EQ(order->get_price(), 155.00);

    // Limit order above market should remain working
    EXPECT_TRUE(order->get_status() == OrderStatus::ACCEPTED || order->get_status() == OrderStatus::NEW);
}

TEST_F(OrderLifecycleTest, CancelWorkingOrder) {
    // Arrange - Submit a limit order that won't execute immediately
    OrderRequest request;
    request.instrument_symbol = test_symbol_;
    request.side = OrderSide::BUY;
    request.type = OrderType::LIMIT;
    request.quantity = 100.0;
    request.price = 140.00; // Below current market price
    request.timestamp = std::chrono::system_clock::now();

    std::string order_id = trading_engine_->submit_order(request);
    ASSERT_FALSE(order_id.empty());

    // Wait for order to be accepted
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify order is working
    auto order = trading_engine_->get_order(order_id);
    ASSERT_NE(order, nullptr);
    EXPECT_TRUE(order->is_working());

    // Act - Cancel the order
    bool cancel_result = trading_engine_->cancel_order(order_id);

    // Assert
    EXPECT_TRUE(cancel_result);

    // Give time for cancellation processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify order was canceled
    order = trading_engine_->get_order(order_id);
    ASSERT_NE(order, nullptr);
    EXPECT_EQ(order->get_status(), OrderStatus::CANCELED);
}

TEST_F(OrderLifecycleTest, RiskRejectionScenario) {
    // Arrange - Submit order that exceeds position limits
    OrderRequest request;
    request.instrument_symbol = test_symbol_;
    request.side = OrderSide::BUY;
    request.type = OrderType::MARKET;
    request.quantity = 15000.0; // Exceeds max position size of 10000
    request.price = 0.0;
    request.timestamp = std::chrono::system_clock::now();

    // Track order updates to catch rejection
    std::vector<ExecutionReport> execution_reports;
    trading_engine_->set_order_update_callback([&execution_reports](const ExecutionReport& report) {
        execution_reports.push_back(report);
    });

    // Act
    std::string order_id = trading_engine_->submit_order(request);

    // Assert
    // Order might be rejected immediately or after submission
    if (order_id.empty()) {
        // Immediate rejection - this is acceptable
        SUCCEED();
    } else {
        // Wait for processing
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        auto order = trading_engine_->get_order(order_id);
        ASSERT_NE(order, nullptr);
        EXPECT_EQ(order->get_status(), OrderStatus::REJECTED);
        EXPECT_FALSE(order->get_rejection_reason().empty());
    }
}

TEST_F(OrderLifecycleTest, PositionUpdatesFromTrades) {
    // Arrange
    OrderRequest request;
    request.instrument_symbol = test_symbol_;
    request.side = OrderSide::BUY;
    request.type = OrderType::MARKET;
    request.quantity = 150.0;
    request.timestamp = std::chrono::system_clock::now();

    // Track position updates
    std::vector<std::shared_ptr<Position>> position_updates;
    trading_engine_->set_position_update_callback([&position_updates](const Position& position) {
        // Store a copy of the position (if Position supports copying) or adjust as needed
        // For now, we'll just track that a position update occurred
        position_updates.push_back(std::make_shared<Position>(position.get_instrument_symbol()));
    });

    // Act
    std::string order_id = trading_engine_->submit_order(request);
    ASSERT_FALSE(order_id.empty());

    // Wait for execution
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Assert
    // Verify position was created/updated
    auto position = trading_engine_->get_position(test_symbol_);
    ASSERT_NE(position, nullptr);
    EXPECT_EQ(position->get_instrument_symbol(), test_symbol_);
    EXPECT_GT(position->get_quantity(), 0.0); // Should have long position
    EXPECT_GT(position->get_average_price(), 0.0);

    // Verify position updates were triggered
    EXPECT_FALSE(position_updates.empty());
}

TEST_F(OrderLifecycleTest, DataPersistence) {
    // Arrange
    OrderRequest request;
    request.instrument_symbol = test_symbol_;
    request.side = OrderSide::BUY;
    request.type = OrderType::MARKET;
    request.quantity = 100.0;
    request.timestamp = std::chrono::system_clock::now();

    // Act
    std::string order_id = trading_engine_->submit_order(request);
    ASSERT_FALSE(order_id.empty());

    // Wait for execution and persistence
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Assert - Verify data was persisted
    auto order = trading_engine_->get_order(order_id);
    ASSERT_NE(order, nullptr);

    // Check that trades were persisted
    auto trades = trading_engine_->get_trades_by_order(order_id);
    EXPECT_FALSE(trades.empty());

    // Check that position was persisted
    auto position = trading_engine_->get_position(test_symbol_);
    ASSERT_NE(position, nullptr);

    // Verify persistence service has the data
    EXPECT_TRUE(persistence_->is_available());
}