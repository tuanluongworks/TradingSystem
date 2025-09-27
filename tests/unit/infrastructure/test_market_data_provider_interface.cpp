#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/market_data/market_data_provider.hpp"
#include "contracts/trading_engine_api.hpp"

using namespace trading;
using ::testing::_;
using ::testing::Return;

class MockMarketDataProvider : public IMarketDataProvider {
public:
    MOCK_METHOD(bool, connect, (), (override));
    MOCK_METHOD(void, disconnect, (), (override));
    MOCK_METHOD(bool, is_connected, (), (const, override));
    MOCK_METHOD(bool, subscribe, (const std::string&), (override));
    MOCK_METHOD(bool, unsubscribe, (const std::string&), (override));
    MOCK_METHOD(std::vector<std::string>, get_subscribed_symbols, (), (const, override));
    MOCK_METHOD(std::shared_ptr<MarketTick>, get_latest_tick, (const std::string&), (const, override));
    MOCK_METHOD(std::vector<std::shared_ptr<MarketTick>>, get_recent_ticks, (const std::string&, int), (const, override));
    MOCK_METHOD(void, set_tick_callback, (std::function<void(const MarketTick&)>), (override));
    MOCK_METHOD(void, set_connection_callback, (std::function<void(bool)>), (override));
};

class MarketDataProviderInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        provider = std::make_unique<MockMarketDataProvider>();
    }

    std::unique_ptr<MockMarketDataProvider> provider;
};

TEST_F(MarketDataProviderInterfaceTest, ConnectionManagementMethodsExist) {
    EXPECT_CALL(*provider, connect())
        .WillOnce(Return(true));
    EXPECT_CALL(*provider, disconnect()).Times(1);
    EXPECT_CALL(*provider, is_connected())
        .WillOnce(Return(true));

    EXPECT_TRUE(provider->connect());
    provider->disconnect();
    EXPECT_TRUE(provider->is_connected());
}

TEST_F(MarketDataProviderInterfaceTest, SubscriptionManagementMethodsExist) {
    EXPECT_CALL(*provider, subscribe("AAPL"))
        .WillOnce(Return(true));
    EXPECT_CALL(*provider, unsubscribe("AAPL"))
        .WillOnce(Return(true));
    EXPECT_CALL(*provider, get_subscribed_symbols())
        .WillOnce(Return(std::vector<std::string>{"AAPL", "GOOGL"}));

    EXPECT_TRUE(provider->subscribe("AAPL"));
    EXPECT_TRUE(provider->unsubscribe("AAPL"));

    auto symbols = provider->get_subscribed_symbols();
    EXPECT_EQ(symbols.size(), 2);
    EXPECT_EQ(symbols[0], "AAPL");
    EXPECT_EQ(symbols[1], "GOOGL");
}

TEST_F(MarketDataProviderInterfaceTest, DataAccessMethodsExist) {
    EXPECT_CALL(*provider, get_latest_tick("AAPL"))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*provider, get_recent_ticks("AAPL", 10))
        .WillOnce(Return(std::vector<std::shared_ptr<MarketTick>>{}));

    auto latest_tick = provider->get_latest_tick("AAPL");
    auto recent_ticks = provider->get_recent_ticks("AAPL", 10);

    EXPECT_EQ(latest_tick, nullptr);
    EXPECT_TRUE(recent_ticks.empty());
}

TEST_F(MarketDataProviderInterfaceTest, CallbackSettersExist) {
    auto tick_callback = [](const MarketTick&) {};
    auto connection_callback = [](bool) {};

    EXPECT_CALL(*provider, set_tick_callback(_)).Times(1);
    EXPECT_CALL(*provider, set_connection_callback(_)).Times(1);

    provider->set_tick_callback(tick_callback);
    provider->set_connection_callback(connection_callback);
}