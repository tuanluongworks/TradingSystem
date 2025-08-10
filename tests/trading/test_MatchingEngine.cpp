#include "../../src/trading/MatchingEngine.h"
#include "../../src/trading/OrderEvents.h"
#include "../../src/infrastructure/LockFreeQueue.h"
#include <thread>
// Bring in test macros
#ifndef TEST
#include "../test_main.cpp" // fallback not ideal; better to extract framework, but minimal change.
#endif
// Remove duplicate main inclusion risk by guarding
#ifdef TEST
TEST(MatchingEngine_SimpleCross) {
    SPSCQueue<TradingEvent> q(128);
    MatchingEngine engine(q); engine.start();
    Order buy; buy.id="B1"; buy.symbol="AAPL"; buy.type=OrderType::BUY; buy.price=101; buy.quantity=100; buy.userId="u1"; q.push(TradingEvent{NewOrderEvent{buy}});
    Order sell; sell.id="S1"; sell.symbol="AAPL"; sell.type=OrderType::SELL; sell.price=100; sell.quantity=100; sell.userId="u2"; q.push(TradingEvent{NewOrderEvent{sell}});
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    auto ob1 = engine.getOrder("B1"); auto os1 = engine.getOrder("S1");
    ASSERT_TRUE(ob1.has_value()); ASSERT_TRUE(os1.has_value());
    ASSERT_EQ((int)ob1->status, (int)OrderStatus::FILLED);
    ASSERT_EQ((int)os1->status, (int)OrderStatus::FILLED);
    engine.stop();
    return true;
}
#endif
