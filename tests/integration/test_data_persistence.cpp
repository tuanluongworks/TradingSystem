#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>

#include "infrastructure/persistence/sqlite_service.hpp"
#include "core/models/order.hpp"
#include "core/models/trade.hpp"
#include "core/models/position.hpp"
#include "core/models/instrument.hpp"
#include "utils/config.hpp"

using namespace trading;

class DataPersistenceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary database file
        test_db_path_ = std::filesystem::temp_directory_path() / "test_trading.db";
        backup_path_ = std::filesystem::temp_directory_path() / "test_backup.sql";

        // Remove any existing test files
        std::filesystem::remove(test_db_path_);
        std::filesystem::remove(backup_path_);

        // Initialize test configuration
        config_ = std::make_shared<TradingSystemConfig>();
        config_->persistence.database_path = test_db_path_.string();
        config_->persistence.auto_backup = true;
        config_->persistence.backup_path = backup_path_.string();

        // Initialize persistence service
        persistence_ = std::make_shared<SQLiteService>(config_->persistence.database_path);
        ASSERT_TRUE(persistence_->initialize());
        ASSERT_TRUE(persistence_->is_available());

        // Create test data
        create_test_data();
    }

    void TearDown() override {
        if (persistence_) {
            persistence_->close();
        }

        // Clean up test files
        std::filesystem::remove(test_db_path_);
        std::filesystem::remove(backup_path_);
    }

    void create_test_data() {
        // Create test instruments
        test_instrument_ = std::make_shared<Instrument>("AAPL", "Apple Inc", InstrumentType::STOCK, 0.01, 1);
        test_instrument_->set_active(true);
        test_instrument_->update_market_data(150.00, 150.05, 150.02);

        // Create test order
        test_order_ = std::make_shared<Order>("ORDER_001", "AAPL", OrderSide::BUY, OrderType::MARKET, 100.0, 0.0);
        test_order_->accept();
        test_order_->fill(100.0, 150.02);

        // Create test trade
        test_trade_ = std::make_shared<Trade>("TRADE_001", "ORDER_001", "AAPL", OrderSide::BUY, 100.0, 150.02, TradeType::FULL_FILL);

        // Create test position
        test_position_ = std::make_shared<Position>("AAPL");
        test_position_->add_trade(100.0, 150.02);
        test_position_->update_unrealized_pnl(150.07); // Assuming price moved up
    }

    std::filesystem::path test_db_path_;
    std::filesystem::path backup_path_;
    std::shared_ptr<TradingSystemConfig> config_;
    std::shared_ptr<SQLiteService> persistence_;

    std::shared_ptr<Instrument> test_instrument_;
    std::shared_ptr<Order> test_order_;
    std::shared_ptr<Trade> test_trade_;
    std::shared_ptr<Position> test_position_;
};

TEST_F(DataPersistenceTest, DatabaseInitialization) {
    // Verify database file was created
    EXPECT_TRUE(std::filesystem::exists(test_db_path_));

    // Verify service status
    EXPECT_TRUE(persistence_->is_available());
    EXPECT_EQ(persistence_->get_status(), "Connected");

    // Verify database file is not empty
    auto file_size = std::filesystem::file_size(test_db_path_);
    EXPECT_GT(file_size, 0);
}

TEST_F(DataPersistenceTest, OrderPersistence) {
    // Act - Save order
    bool save_result = persistence_->save_order(*test_order_);
    EXPECT_TRUE(save_result);

    // Give time for async operations
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Assert - Load orders and verify
    auto today = std::chrono::system_clock::now();
    auto loaded_orders = persistence_->load_orders_by_date(today);

    ASSERT_FALSE(loaded_orders.empty());

    bool found_order = false;
    for (const auto& order : loaded_orders) {
        if (order->get_order_id() == test_order_->get_order_id()) {
            found_order = true;
            EXPECT_EQ(order->get_instrument_symbol(), test_order_->get_instrument_symbol());
            EXPECT_EQ(order->get_side(), test_order_->get_side());
            EXPECT_EQ(order->get_type(), test_order_->get_type());
            EXPECT_EQ(order->get_quantity(), test_order_->get_quantity());
            EXPECT_EQ(order->get_status(), test_order_->get_status());
            EXPECT_EQ(order->get_filled_quantity(), test_order_->get_filled_quantity());
            break;
        }
    }

    EXPECT_TRUE(found_order) << "Saved order not found in loaded orders";
}

TEST_F(DataPersistenceTest, TradePersistence) {
    // Act - Save trade
    bool save_result = persistence_->save_trade(*test_trade_);
    EXPECT_TRUE(save_result);

    // Give time for async operations
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Assert - Load trades and verify
    auto today = std::chrono::system_clock::now();
    auto loaded_trades = persistence_->load_trades_by_date(today);

    ASSERT_FALSE(loaded_trades.empty());

    bool found_trade = false;
    for (const auto& trade : loaded_trades) {
        if (trade->get_trade_id() == test_trade_->get_trade_id()) {
            found_trade = true;
            EXPECT_EQ(trade->get_order_id(), test_trade_->get_order_id());
            EXPECT_EQ(trade->get_instrument_symbol(), test_trade_->get_instrument_symbol());
            EXPECT_EQ(trade->get_side(), test_trade_->get_side());
            EXPECT_EQ(trade->get_quantity(), test_trade_->get_quantity());
            EXPECT_EQ(trade->get_price(), test_trade_->get_price());
            break;
        }
    }

    EXPECT_TRUE(found_trade) << "Saved trade not found in loaded trades";
}

TEST_F(DataPersistenceTest, PositionPersistence) {
    // Act - Save position
    bool save_result = persistence_->update_position(*test_position_);
    EXPECT_TRUE(save_result);

    // Give time for async operations
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Assert - Load positions and verify
    auto loaded_positions = persistence_->load_all_positions();

    ASSERT_FALSE(loaded_positions.empty());

    bool found_position = false;
    for (const auto& position : loaded_positions) {
        if (position->get_instrument_symbol() == test_position_->get_instrument_symbol()) {
            found_position = true;
            EXPECT_EQ(position->get_quantity(), test_position_->get_quantity());
            EXPECT_EQ(position->get_average_price(), test_position_->get_average_price());
            EXPECT_EQ(position->get_realized_pnl(), test_position_->get_realized_pnl());
            EXPECT_EQ(position->get_unrealized_pnl(), test_position_->get_unrealized_pnl());
            break;
        }
    }

    EXPECT_TRUE(found_position) << "Saved position not found in loaded positions";
}

TEST_F(DataPersistenceTest, MultipleRecordsPersistence) {
    // Arrange - Create multiple records
    std::vector<std::shared_ptr<Order>> orders;
    std::vector<std::shared_ptr<Trade>> trades;

    for (int i = 0; i < 5; ++i) {
        // Create order
        OrderSide side = (i % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;
        double quantity = 100.0 + i * 50.0;
        auto order = std::make_shared<Order>("ORDER_" + std::to_string(i), "AAPL", side, OrderType::MARKET, quantity, 0.0);
        order->accept();
        order->fill(quantity, 150.0 + i * 0.5);
        orders.push_back(order);

        // Create corresponding trade
        auto trade = std::make_shared<Trade>("TRADE_" + std::to_string(i), order->get_order_id(), "AAPL", side, quantity, 150.0 + i * 0.5, TradeType::FULL_FILL);
        trades.push_back(trade);
    }

    // Act - Save all records
    for (const auto& order : orders) {
        EXPECT_TRUE(persistence_->save_order(*order));
    }

    for (const auto& trade : trades) {
        EXPECT_TRUE(persistence_->save_trade(*trade));
    }

    // Give time for async operations
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Assert - Verify all records were saved
    auto today = std::chrono::system_clock::now();
    auto loaded_orders = persistence_->load_orders_by_date(today);
    auto loaded_trades = persistence_->load_trades_by_date(today);

    EXPECT_EQ(loaded_orders.size(), orders.size());
    EXPECT_EQ(loaded_trades.size(), trades.size());
}

TEST_F(DataPersistenceTest, DatabaseBackup) {
    // Arrange - Save some data first
    ASSERT_TRUE(persistence_->save_order(*test_order_));
    ASSERT_TRUE(persistence_->save_trade(*test_trade_));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Act - Create backup
    bool backup_result = persistence_->backup_to_file(backup_path_.string());
    EXPECT_TRUE(backup_result);

    // Assert - Verify backup file was created
    EXPECT_TRUE(std::filesystem::exists(backup_path_));

    // Verify backup file is not empty
    auto backup_size = std::filesystem::file_size(backup_path_);
    EXPECT_GT(backup_size, 0);

    // Verify backup contains SQL content
    std::ifstream backup_file(backup_path_);
    std::string backup_content((std::istreambuf_iterator<char>(backup_file)),
                               std::istreambuf_iterator<char>());
    EXPECT_FALSE(backup_content.empty());
    EXPECT_NE(backup_content.find("INSERT"), std::string::npos);
}

TEST_F(DataPersistenceTest, DatabaseRestore) {
    // Arrange - Create backup with data
    ASSERT_TRUE(persistence_->save_order(*test_order_));
    ASSERT_TRUE(persistence_->save_trade(*test_trade_));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ASSERT_TRUE(persistence_->backup_to_file(backup_path_.string()));

    // Clear the database by recreating it
    persistence_->close();
    std::filesystem::remove(test_db_path_);

    persistence_ = std::make_shared<SQLiteService>(config_->persistence.database_path);
    ASSERT_TRUE(persistence_->initialize());

    // Verify database is empty
    auto today = std::chrono::system_clock::now();
    auto empty_orders = persistence_->load_orders_by_date(today);
    auto empty_trades = persistence_->load_trades_by_date(today);
    EXPECT_TRUE(empty_orders.empty());
    EXPECT_TRUE(empty_trades.empty());

    // Act - Restore from backup
    bool restore_result = persistence_->restore_from_file(backup_path_.string());
    EXPECT_TRUE(restore_result);

    // Assert - Verify data was restored
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto restored_orders = persistence_->load_orders_by_date(today);
    auto restored_trades = persistence_->load_trades_by_date(today);

    EXPECT_FALSE(restored_orders.empty());
    EXPECT_FALSE(restored_trades.empty());

    // Verify specific data
    bool found_order = false;
    for (const auto& order : restored_orders) {
        if (order->get_order_id() == test_order_->get_order_id()) {
            found_order = true;
            break;
        }
    }
    EXPECT_TRUE(found_order);
}

TEST_F(DataPersistenceTest, ConcurrentWrites) {
    // Test concurrent write operations
    const int num_threads = 4;
    const int records_per_thread = 10;
    std::vector<std::thread> threads;
    std::atomic<int> success_count(0);

    // Act - Launch concurrent write operations
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, &success_count]() {
            for (int i = 0; i < 10; ++i) {
                auto order = std::make_shared<Order>("ORDER_" + std::to_string(t) + "_" + std::to_string(i), "AAPL", OrderSide::BUY, OrderType::MARKET, 100.0, 0.0);
                order->accept();
                order->fill(100.0, 150.0);

                if (persistence_->save_order(*order)) {
                    success_count.fetch_add(1);
                }
            }
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    // Assert - All writes should succeed
    EXPECT_EQ(success_count.load(), num_threads * records_per_thread);

    // Verify all records were saved
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    auto today = std::chrono::system_clock::now();
    auto all_orders = persistence_->load_orders_by_date(today);
    EXPECT_EQ(all_orders.size(), num_threads * records_per_thread);
}

TEST_F(DataPersistenceTest, LargeDatasetPerformance) {
    // Test performance with larger datasets
    const int num_records = 1000;
    std::vector<std::shared_ptr<Trade>> trades;

    // Create test trades
    for (int i = 0; i < num_records; ++i) {
        OrderSide side = (i % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;
        auto trade = std::make_shared<Trade>("PERF_TRADE_" + std::to_string(i), "PERF_ORDER_" + std::to_string(i), "AAPL", side, 100.0, 150.0 + (i % 100) * 0.01, TradeType::FULL_FILL);
        trades.push_back(trade);
    }

    // Act - Time the save operations
    auto start_time = std::chrono::high_resolution_clock::now();

    for (const auto& trade : trades) {
        EXPECT_TRUE(persistence_->save_trade(*trade));
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Assert - Should complete in reasonable time
    EXPECT_LT(duration.count(), 5000) // Less than 5 seconds
        << "Performance test took too long: " << duration.count() << " ms";

    // Wait for async operations to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Verify all records were saved
    auto today = std::chrono::system_clock::now();
    auto loaded_trades = persistence_->load_trades_by_date(today);
    EXPECT_EQ(loaded_trades.size(), num_records);
}

TEST_F(DataPersistenceTest, ErrorHandling) {
    // Test behavior with invalid database path
    auto invalid_config = std::make_shared<TradingSystemConfig>();
    invalid_config->persistence.database_path = "/invalid/path/to/database.db";

    auto invalid_persistence = std::make_shared<SQLiteService>(invalid_config->persistence.database_path);

    // Should fail to initialize with invalid path
    bool init_result = invalid_persistence->initialize();
    EXPECT_FALSE(init_result);
    EXPECT_FALSE(invalid_persistence->is_available());

    // Test saving to invalid persistence service
    bool save_result = invalid_persistence->save_order(*test_order_);
    EXPECT_FALSE(save_result);
}

TEST_F(DataPersistenceTest, TransactionIntegrity) {
    // Test that related data (order + trade) maintains integrity

    // Save order first
    ASSERT_TRUE(persistence_->save_order(*test_order_));

    // Save related trade
    ASSERT_TRUE(persistence_->save_trade(*test_trade_));

    // Update position based on trade
    ASSERT_TRUE(persistence_->update_position(*test_position_));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify all related data exists
    auto today = std::chrono::system_clock::now();
    auto orders = persistence_->load_orders_by_date(today);
    auto trades = persistence_->load_trades_by_date(today);
    auto positions = persistence_->load_all_positions();

    EXPECT_FALSE(orders.empty());
    EXPECT_FALSE(trades.empty());
    EXPECT_FALSE(positions.empty());

    // Verify relationships
    bool found_matching_trade = false;
    for (const auto& trade : trades) {
        if (trade->get_order_id() == test_order_->get_order_id()) {
            found_matching_trade = true;
            EXPECT_EQ(trade->get_instrument_symbol(), test_order_->get_instrument_symbol());
            break;
        }
    }
    EXPECT_TRUE(found_matching_trade);
}

