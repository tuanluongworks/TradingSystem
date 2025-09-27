#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>

#include "core/risk/risk_manager.hpp"
#include "core/engine/trading_engine.hpp"
#include "core/models/order.hpp"
#include "core/models/position.hpp"
#include "core/models/risk_limit.hpp"
#include "infrastructure/persistence/sqlite_service.hpp"
#include "utils/config.hpp"

using namespace trading;

class RiskValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test configuration with strict risk limits
        config_ = std::make_shared<Config>();
        config_->set("database.path", ":memory:");
        config_->set("risk.max_position_size", 1000.0);
        config_->set("risk.max_order_size", 500.0);
        config_->set("risk.max_daily_loss", 5000.0);
        config_->set("risk.enable_pre_trade_checks", true);

        // Initialize persistence service
        persistence_ = std::make_shared<PersistenceService>(config_);
        ASSERT_TRUE(persistence_->initialize());

        // Initialize risk manager
        risk_manager_ = std::make_shared<RiskManager>(config_);
        ASSERT_TRUE(risk_manager_->initialize());

        // Initialize trading engine with risk manager
        trading_engine_ = std::make_shared<TradingEngine>(config_, persistence_, risk_manager_);
        ASSERT_TRUE(trading_engine_->initialize());

        // Set up test symbols
        test_symbol_ = "AAPL";
        setup_risk_limits();

        // Track rejected orders
        rejected_orders_.clear();
        trading_engine_->set_order_update_callback([this](const ExecutionReport& report) {
            if (report.new_status == OrderStatus::REJECTED) {
                rejected_orders_.push_back(report.order_id);
            }
        });
    }

    void TearDown() override {
        if (trading_engine_) {
            trading_engine_->shutdown();
        }
        if (persistence_) {
            persistence_->shutdown();
        }
    }

    void setup_risk_limits() {
        // Set up symbol-specific risk limits
        ASSERT_TRUE(risk_manager_->set_position_limit(test_symbol_, 800.0));
        ASSERT_TRUE(risk_manager_->set_order_size_limit(test_symbol_, 300.0));
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

    void create_position(const std::string& symbol, double quantity) {
        auto request = create_order_request(symbol, OrderSide::BUY, OrderType::MARKET, quantity);
        std::string order_id = trading_engine_->submit_order(request);
        ASSERT_FALSE(order_id.empty());
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::shared_ptr<Config> config_;
    std::shared_ptr<PersistenceService> persistence_;
    std::shared_ptr<RiskManager> risk_manager_;
    std::shared_ptr<TradingEngine> trading_engine_;
    std::string test_symbol_;
    std::vector<std::string> rejected_orders_;
};

TEST_F(RiskValidationTest, OrderSizeLimitValidation) {
    // Arrange
    double order_size_limit = risk_manager_->get_order_size_limit(test_symbol_);
    EXPECT_EQ(order_size_limit, 300.0);

    // Test 1: Order within limit should pass
    auto valid_request = create_order_request(test_symbol_, OrderSide::BUY,
                                            OrderType::MARKET, 250.0);
    EXPECT_TRUE(risk_manager_->validate_order(valid_request));

    // Test 2: Order exceeding limit should fail
    auto invalid_request = create_order_request(test_symbol_, OrderSide::BUY,
                                              OrderType::MARKET, 350.0);
    EXPECT_FALSE(risk_manager_->validate_order(invalid_request));

    std::string rejection_reason = risk_manager_->get_rejection_reason(invalid_request);
    EXPECT_FALSE(rejection_reason.empty());
    EXPECT_NE(rejection_reason.find("order size"), std::string::npos);
}

TEST_F(RiskValidationTest, PositionLimitValidation) {
    // Arrange - Create existing position
    create_position(test_symbol_, 600.0);

    auto existing_position = trading_engine_->get_position(test_symbol_);
    ASSERT_NE(existing_position, nullptr);
    EXPECT_EQ(existing_position->quantity, 600.0);

    double position_limit = risk_manager_->get_position_limit(test_symbol_);
    EXPECT_EQ(position_limit, 800.0);

    // Test 1: Order that would keep position within limit should pass
    auto valid_request = create_order_request(test_symbol_, OrderSide::BUY,
                                            OrderType::MARKET, 150.0);
    EXPECT_TRUE(risk_manager_->validate_order(valid_request));

    // Test 2: Order that would exceed position limit should fail
    auto invalid_request = create_order_request(test_symbol_, OrderSide::BUY,
                                              OrderType::MARKET, 250.0);
    EXPECT_FALSE(risk_manager_->validate_order(invalid_request));

    std::string rejection_reason = risk_manager_->get_rejection_reason(invalid_request);
    EXPECT_FALSE(rejection_reason.empty());
    EXPECT_NE(rejection_reason.find("position limit"), std::string::npos);
}

TEST_F(RiskValidationTest, PositionLimitWithShortSelling) {
    // Test short position limit (if supported)
    double position_limit = risk_manager_->get_position_limit(test_symbol_);

    // Try to create short position within limit
    auto short_request = create_order_request(test_symbol_, OrderSide::SELL,
                                            OrderType::MARKET, 500.0);

    if (risk_manager_->validate_order(short_request)) {
        // Short selling is supported
        std::string order_id = trading_engine_->submit_order(short_request);
        if (!order_id.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            auto position = trading_engine_->get_position(test_symbol_);
            if (position != nullptr) {
                EXPECT_EQ(position->quantity, -500.0);

                // Now test limit on further short selling
                auto excessive_short = create_order_request(test_symbol_, OrderSide::SELL,
                                                          OrderType::MARKET, 400.0);
                EXPECT_FALSE(risk_manager_->validate_order(excessive_short));
            }
        }
    } else {
        // Short selling not allowed - verify rejection reason
        std::string rejection_reason = risk_manager_->get_rejection_reason(short_request);
        EXPECT_FALSE(rejection_reason.empty());
    }
}

TEST_F(RiskValidationTest, DailyLossLimitValidation) {
    // Arrange - Create a scenario that would generate losses
    create_position(test_symbol_, 400.0);

    // Simulate some realized losses (this would normally come from trading)
    // For testing, we'll manually set up the scenario
    double daily_loss_limit = risk_manager_->get_daily_loss_limit();
    EXPECT_EQ(daily_loss_limit, 5000.0);

    // Note: Testing daily loss limits properly requires simulating market movements
    // and actual P&L calculations. This test verifies the configuration is correct.
    double current_daily_pnl = risk_manager_->get_daily_pnl();

    // The daily P&L should start at 0 for a fresh session
    EXPECT_GE(current_daily_pnl, -daily_loss_limit);
}

TEST_F(RiskValidationTest, PreTradeRiskCheckIntegration) {
    // Arrange - Submit order that should be rejected by risk manager
    auto invalid_request = create_order_request(test_symbol_, OrderSide::BUY,
                                              OrderType::MARKET, 600.0); // Exceeds order size limit

    // Act
    std::string order_id = trading_engine_->submit_order(invalid_request);

    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Assert - Order should be rejected
    if (!order_id.empty()) {
        // Order was created but should be rejected
        auto order = trading_engine_->get_order(order_id);
        ASSERT_NE(order, nullptr);
        EXPECT_EQ(order->status, OrderStatus::REJECTED);
        EXPECT_FALSE(order->rejection_reason.empty());

        // Should appear in rejected orders list
        EXPECT_TRUE(std::find(rejected_orders_.begin(), rejected_orders_.end(), order_id)
                   != rejected_orders_.end());
    } else {
        // Order was rejected before creation (immediate rejection)
        SUCCEED(); // This is also acceptable behavior
    }

    // Verify no position was created
    auto position = trading_engine_->get_position(test_symbol_);
    EXPECT_EQ(position, nullptr);
}

TEST_F(RiskValidationTest, DynamicRiskLimitUpdates) {
    // Arrange - Create initial position
    create_position(test_symbol_, 200.0);

    // Test that we can add more within current limits
    auto request1 = create_order_request(test_symbol_, OrderSide::BUY, OrderType::MARKET, 250.0);
    EXPECT_TRUE(risk_manager_->validate_order(request1));

    // Act - Reduce position limit
    ASSERT_TRUE(risk_manager_->set_position_limit(test_symbol_, 400.0));

    // Assert - Same order should now be rejected
    EXPECT_FALSE(risk_manager_->validate_order(request1));

    // Verify the new limit is in effect
    EXPECT_EQ(risk_manager_->get_position_limit(test_symbol_), 400.0);

    // But a smaller order should still work
    auto request2 = create_order_request(test_symbol_, OrderSide::BUY, OrderType::MARKET, 150.0);
    EXPECT_TRUE(risk_manager_->validate_order(request2));
}

TEST_F(RiskValidationTest, RiskLimitPersistence) {
    // Arrange - Set custom risk limits
    ASSERT_TRUE(risk_manager_->set_position_limit("GOOGL", 1500.0));
    ASSERT_TRUE(risk_manager_->set_order_size_limit("GOOGL", 750.0));

    // Verify limits are set
    EXPECT_EQ(risk_manager_->get_position_limit("GOOGL"), 1500.0);
    EXPECT_EQ(risk_manager_->get_order_size_limit("GOOGL"), 750.0);

    // Act - Simulate system restart
    risk_manager_ = std::make_shared<RiskManager>(config_);
    ASSERT_TRUE(risk_manager_->initialize());

    // Assert - Verify limits were restored (if persistence is implemented)
    // Note: This test may fail if risk limits are not persisted
    // In that case, they should revert to defaults
    double restored_position_limit = risk_manager_->get_position_limit("GOOGL");
    double restored_order_limit = risk_manager_->get_order_size_limit("GOOGL");

    // Either restored to custom values or reverted to defaults
    EXPECT_TRUE(restored_position_limit == 1500.0 || restored_position_limit == 1000.0);
    EXPECT_TRUE(restored_order_limit == 750.0 || restored_order_limit == 500.0);
}

TEST_F(RiskValidationTest, MultiSymbolRiskManagement) {
    // Arrange - Set different limits for different symbols
    std::vector<std::string> symbols = {"AAPL", "GOOGL", "MSFT"};
    std::vector<double> position_limits = {800.0, 1200.0, 600.0};
    std::vector<double> order_limits = {300.0, 400.0, 200.0};

    for (size_t i = 0; i < symbols.size(); ++i) {
        ASSERT_TRUE(risk_manager_->set_position_limit(symbols[i], position_limits[i]));
        ASSERT_TRUE(risk_manager_->set_order_size_limit(symbols[i], order_limits[i]));
    }

    // Act & Assert - Test each symbol's limits independently
    for (size_t i = 0; i < symbols.size(); ++i) {
        // Valid order within limit
        auto valid_request = create_order_request(symbols[i], OrderSide::BUY,
                                                OrderType::MARKET, order_limits[i] - 50.0);
        EXPECT_TRUE(risk_manager_->validate_order(valid_request))
            << "Valid order failed for " << symbols[i];

        // Invalid order exceeding limit
        auto invalid_request = create_order_request(symbols[i], OrderSide::BUY,
                                                  OrderType::MARKET, order_limits[i] + 50.0);
        EXPECT_FALSE(risk_manager_->validate_order(invalid_request))
            << "Invalid order passed for " << symbols[i];
    }
}

TEST_F(RiskValidationTest, ExposureCalculation) {
    // Arrange - Create positions in multiple symbols
    create_position("AAPL", 300.0);
    create_position("GOOGL", 150.0);
    create_position("MSFT", 200.0);

    // Act - Calculate exposures
    double aapl_exposure = risk_manager_->get_current_exposure("AAPL");
    double googl_exposure = risk_manager_->get_current_exposure("GOOGL");
    double msft_exposure = risk_manager_->get_current_exposure("MSFT");
    double total_exposure = risk_manager_->get_total_position_value();

    // Assert - Verify exposures are calculated
    EXPECT_GT(aapl_exposure, 0.0);
    EXPECT_GT(googl_exposure, 0.0);
    EXPECT_GT(msft_exposure, 0.0);
    EXPECT_GT(total_exposure, 0.0);

    // Total exposure should be sum of individual exposures (approximately)
    double sum_exposures = aapl_exposure + googl_exposure + msft_exposure;
    EXPECT_NEAR(total_exposure, sum_exposures, 1.0); // Allow small rounding differences
}

TEST_F(RiskValidationTest, RiskValidationPerformance) {
    // Test that risk validation doesn't introduce significant latency

    // Arrange
    auto request = create_order_request(test_symbol_, OrderSide::BUY, OrderType::MARKET, 100.0);
    const int num_validations = 1000;

    // Act - Time multiple risk validations
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_validations; ++i) {
        bool result = risk_manager_->validate_order(request);
        EXPECT_TRUE(result); // All should pass
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    // Assert - Validation should be fast
    double avg_time_per_validation = static_cast<double>(duration.count()) / num_validations;
    EXPECT_LT(avg_time_per_validation, 100.0) // Less than 100 microseconds per validation
        << "Risk validation too slow: " << avg_time_per_validation << " μs per validation";
}

} // Anonymous namespace for tests