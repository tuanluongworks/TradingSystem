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
        config_ = std::make_shared<Config>();
        config_->set("database.path", test_db_path_.string());
        config_->set("database.backup_enabled", true);
        config_->set("database.backup_path", backup_path_.string());

        // Initialize persistence service
        persistence_ = std::make_shared<PersistenceService>(config_);
        ASSERT_TRUE(persistence_->initialize());
        ASSERT_TRUE(persistence_->is_available());

        // Create test data
        create_test_data();
    }

    void TearDown() override {
        if (persistence_) {
            persistence_->shutdown();
        }

        // Clean up test files
        std::filesystem::remove(test_db_path_);
        std::filesystem::remove(backup_path_);
    }

    void create_test_data() {
        // Create test instruments
        test_instrument_ = std::make_shared<Instrument>();
        test_instrument_->symbol = "AAPL";
        test_instrument_->name = "Apple Inc";
        test_instrument_->type = InstrumentType::STOCK;
        test_instrument_->tick_size = 0.01;
        test_instrument_->lot_size = 1;
        test_instrument_->is_active = true;
        test_instrument_->bid_price = 150.00;
        test_instrument_->ask_price = 150.05;
        test_instrument_->last_price = 150.02;

        // Create test order
        test_order_ = std::make_shared<Order>();
        test_order_->order_id = "ORDER_001";
        test_order_->instrument_symbol = "AAPL";
        test_order_->side = OrderSide::BUY;
        test_order_->type = OrderType::MARKET;
        test_order_->quantity = 100.0;
        test_order_->price = 0.0;
        test_order_->status = OrderStatus::FILLED;
        test_order_->filled_quantity = 100.0;
        test_order_->remaining_quantity = 0.0;
        test_order_->created_time = std::chrono::system_clock::now();
        test_order_->last_modified = test_order_->created_time;

        // Create test trade
        test_trade_ = std::make_shared<Trade>();
        test_trade_->trade_id = "TRADE_001";
        test_trade_->order_id = "ORDER_001";
        test_trade_->instrument_symbol = "AAPL";
        test_trade_->side = OrderSide::BUY;
        test_trade_->quantity = 100.0;
        test_trade_->price = 150.02;
        test_trade_->execution_time = std::chrono::system_clock::now();
        test_trade_->type = TradeType::FULL_FILL;

        // Create test position
        test_position_ = std::make_shared<Position>();
        test_position_->instrument_symbol = "AAPL";
        test_position_->quantity = 100.0;
        test_position_->average_price = 150.02;
        test_position_->realized_pnl = 0.0;
        test_position_->unrealized_pnl = 5.0; // Assuming price moved up
        test_position_->last_updated = std::chrono::system_clock::now();
    }

    std::filesystem::path test_db_path_;
    std::filesystem::path backup_path_;
    std::shared_ptr<Config> config_;
    std::shared_ptr<PersistenceService> persistence_;

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
        if (order->order_id == test_order_->order_id) {
            found_order = true;
            EXPECT_EQ(order->instrument_symbol, test_order_->instrument_symbol);
            EXPECT_EQ(order->side, test_order_->side);
            EXPECT_EQ(order->type, test_order_->type);
            EXPECT_EQ(order->quantity, test_order_->quantity);
            EXPECT_EQ(order->status, test_order_->status);
            EXPECT_EQ(order->filled_quantity, test_order_->filled_quantity);
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
        if (trade->trade_id == test_trade_->trade_id) {
            found_trade = true;
            EXPECT_EQ(trade->order_id, test_trade_->order_id);
            EXPECT_EQ(trade->instrument_symbol, test_trade_->instrument_symbol);
            EXPECT_EQ(trade->side, test_trade_->side);
            EXPECT_EQ(trade->quantity, test_trade_->quantity);
            EXPECT_EQ(trade->price, test_trade_->price);
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
        if (position->instrument_symbol == test_position_->instrument_symbol) {
            found_position = true;
            EXPECT_EQ(position->quantity, test_position_->quantity);
            EXPECT_EQ(position->average_price, test_position_->average_price);
            EXPECT_EQ(position->realized_pnl, test_position_->realized_pnl);
            EXPECT_EQ(position->unrealized_pnl, test_position_->unrealized_pnl);
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
        auto order = std::make_shared<Order>();
        order->order_id = "ORDER_" + std::to_string(i);
        order->instrument_symbol = "AAPL";
        order->side = (i % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;
        order->type = OrderType::MARKET;
        order->quantity = 100.0 + i * 50.0;
        order->status = OrderStatus::FILLED;
        order->filled_quantity = order->quantity;
        order->remaining_quantity = 0.0;
        order->created_time = std::chrono::system_clock::now();
        order->last_modified = order->created_time;
        orders.push_back(order);

        // Create corresponding trade
        auto trade = std::make_shared<Trade>();
        trade->trade_id = "TRADE_" + std::to_string(i);
        trade->order_id = order->order_id;
        trade->instrument_symbol = "AAPL";
        trade->side = order->side;
        trade->quantity = order->quantity;
        trade->price = 150.0 + i * 0.5;
        trade->execution_time = std::chrono::system_clock::now();
        trade->type = TradeType::FULL_FILL;
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
    persistence_->shutdown();
    std::filesystem::remove(test_db_path_);

    persistence_ = std::make_shared<PersistenceService>(config_);
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
        if (order->order_id == test_order_->order_id) {
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
        threads.emplace_back([this, t, records_per_thread, &success_count]() {
            for (int i = 0; i < records_per_thread; ++i) {
                auto order = std::make_shared<Order>();
                order->order_id = "ORDER_" + std::to_string(t) + "_" + std::to_string(i);
                order->instrument_symbol = "AAPL";
                order->side = OrderSide::BUY;
                order->type = OrderType::MARKET;
                order->quantity = 100.0;
                order->status = OrderStatus::FILLED;
                order->filled_quantity = 100.0;
                order->remaining_quantity = 0.0;
                order->created_time = std::chrono::system_clock::now();
                order->last_modified = order->created_time;

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
        auto trade = std::make_shared<Trade>();
        trade->trade_id = "PERF_TRADE_" + std::to_string(i);
        trade->order_id = "PERF_ORDER_" + std::to_string(i);
        trade->instrument_symbol = "AAPL";
        trade->side = (i % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;
        trade->quantity = 100.0;
        trade->price = 150.0 + (i % 100) * 0.01;
        trade->execution_time = std::chrono::system_clock::now();
        trade->type = TradeType::FULL_FILL;
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
    auto invalid_config = std::make_shared<Config>();
    invalid_config->set("database.path", "/invalid/path/to/database.db");

    auto invalid_persistence = std::make_shared<PersistenceService>(invalid_config);

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
        if (trade->order_id == test_order_->order_id) {
            found_matching_trade = true;
            EXPECT_EQ(trade->instrument_symbol, test_order_->instrument_symbol);
            break;
        }
    }
    EXPECT_TRUE(found_matching_trade);
}

} // Anonymous namespace for tests