#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/persistence/sqlite_service.hpp"
#include "contracts/trading_engine_api.hpp"

using namespace trading;
using ::testing::_;
using ::testing::Return;

class MockPersistenceService : public IPersistenceService {
public:
    MOCK_METHOD(bool, save_trade, (const Trade&), (override));
    MOCK_METHOD(bool, save_order, (const Order&), (override));
    MOCK_METHOD(bool, update_position, (const Position&), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<Trade>>, load_trades_by_date, (const std::chrono::system_clock::time_point&), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<Order>>, load_orders_by_date, (const std::chrono::system_clock::time_point&), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<Position>>, load_all_positions, (), (override));
    MOCK_METHOD(bool, backup_to_file, (const std::string&), (override));
    MOCK_METHOD(bool, restore_from_file, (const std::string&), (override));
    MOCK_METHOD(bool, is_available, (), (const, override));
    MOCK_METHOD(std::string, get_status, (), (const, override));
};

class PersistenceServiceInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        service = std::make_unique<MockPersistenceService>();
    }

    std::unique_ptr<MockPersistenceService> service;
};

TEST_F(PersistenceServiceInterfaceTest, SaveMethodsExist) {
    Trade test_trade;
    Order test_order;
    Position test_position;

    EXPECT_CALL(*service, save_trade(_))
        .WillOnce(Return(true));
    EXPECT_CALL(*service, save_order(_))
        .WillOnce(Return(true));
    EXPECT_CALL(*service, update_position(_))
        .WillOnce(Return(true));

    EXPECT_TRUE(service->save_trade(test_trade));
    EXPECT_TRUE(service->save_order(test_order));
    EXPECT_TRUE(service->update_position(test_position));
}

TEST_F(PersistenceServiceInterfaceTest, LoadMethodsExist) {
    auto now = std::chrono::system_clock::now();

    EXPECT_CALL(*service, load_trades_by_date(_))
        .WillOnce(Return(std::vector<std::shared_ptr<Trade>>{}));
    EXPECT_CALL(*service, load_orders_by_date(_))
        .WillOnce(Return(std::vector<std::shared_ptr<Order>>{}));
    EXPECT_CALL(*service, load_all_positions())
        .WillOnce(Return(std::vector<std::shared_ptr<Position>>{}));

    auto trades = service->load_trades_by_date(now);
    auto orders = service->load_orders_by_date(now);
    auto positions = service->load_all_positions();

    EXPECT_TRUE(trades.empty());
    EXPECT_TRUE(orders.empty());
    EXPECT_TRUE(positions.empty());
}

TEST_F(PersistenceServiceInterfaceTest, BackupMethodsExist) {
    EXPECT_CALL(*service, backup_to_file("/tmp/backup.db"))
        .WillOnce(Return(true));
    EXPECT_CALL(*service, restore_from_file("/tmp/backup.db"))
        .WillOnce(Return(true));

    EXPECT_TRUE(service->backup_to_file("/tmp/backup.db"));
    EXPECT_TRUE(service->restore_from_file("/tmp/backup.db"));
}

TEST_F(PersistenceServiceInterfaceTest, HealthCheckMethodsExist) {
    EXPECT_CALL(*service, is_available())
        .WillOnce(Return(true));
    EXPECT_CALL(*service, get_status())
        .WillOnce(Return("Connected"));

    EXPECT_TRUE(service->is_available());
    EXPECT_EQ(service->get_status(), "Connected");
}