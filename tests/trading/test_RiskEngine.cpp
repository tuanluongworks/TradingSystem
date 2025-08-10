#include "../../src/trading/RiskEngine.h"
#include <string>

TEST(RiskEngine_PreValidateWithinLimits) {
    RiskLimits limits; // defaults
    RiskEngine engine(limits);
    Order o; o.symbol="AAPL"; o.type=OrderType::BUY; o.quantity=100; o.price=150; o.userId="u1";
    std::string reason;
    bool ok = engine.preValidateNewOrder(o, reason);
    ASSERT_TRUE(ok);
    ASSERT_TRUE(reason.empty());
    return true;
}

TEST(RiskEngine_RejectNotional) {
    RiskLimits limits; limits.maxNotionalPerSymbol = 100000; // 100k
    RiskEngine engine(limits);
    Order o; o.symbol="AAPL"; o.type=OrderType::BUY; o.quantity=10000; o.price=20; // 200k
    std::string reason; bool ok = engine.preValidateNewOrder(o, reason);
    ASSERT_FALSE(ok); ASSERT_TRUE(reason.find("notional")!=std::string::npos);
    return true;
}

TEST(RiskEngine_PositionLimit) {
    RiskLimits limits; limits.maxPositionPerSymbol = 500; // tighten
    RiskEngine engine(limits);
    // First acceptable
    Order o1; o1.symbol="AAPL"; o1.type=OrderType::BUY; o1.quantity=400; o1.price=10; o1.userId="u1";
    std::string reason; ASSERT_TRUE(engine.preValidateNewOrder(o1, reason));
    engine.onOrderExecuted(o1);
    // Second would exceed
    Order o2; o2.symbol="AAPL"; o2.type=OrderType::BUY; o2.quantity=200; o2.price=10; o2.userId="u1";
    reason.clear(); ASSERT_FALSE(engine.preValidateNewOrder(o2, reason));
    return true;
}

TEST(RiskEngine_PortfolioNotionalLimit) {
    RiskLimits limits; limits.maxPortfolioNotional = 10000; // small
    RiskEngine engine(limits);
    Order o1; o1.symbol="AAPL"; o1.type=OrderType::BUY; o1.quantity=50; o1.price=100; o1.userId="u1"; // 5000
    std::string reason; ASSERT_TRUE(engine.preValidateNewOrder(o1, reason)); engine.onOrderExecuted(o1);
    Order o2; o2.symbol="MSFT"; o2.type=OrderType::BUY; o2.quantity=60; o2.price=100; o2.userId="u1"; // +6000 => 11000
    reason.clear(); ASSERT_FALSE(engine.preValidateNewOrder(o2, reason));
    return true;
}

TEST(RiskEngine_AvgPriceAndFlip) {
    RiskEngine engine; // defaults
    Order b1; b1.symbol="AAPL"; b1.type=OrderType::BUY; b1.quantity=100; b1.price=10; b1.userId="u1"; engine.onOrderExecuted(b1);
    Order b2; b2.symbol="AAPL"; b2.type=OrderType::BUY; b2.quantity=100; b2.price=20; b2.userId="u1"; engine.onOrderExecuted(b2);
    // Exposure should be (200 * weighted avg 15) = 3000
    double exp = engine.currentSymbolExposure("AAPL"); ASSERT_EQ(exp, 3000);
    // Now sell more than position to flip
    Order s1; s1.symbol="AAPL"; s1.type=OrderType::SELL; s1.quantity=250; s1.price=30; s1.userId="u1"; engine.onOrderExecuted(s1);
    // New position: -50 @ 30 (since flipped)
    double exp2 = engine.currentSymbolExposure("AAPL"); ASSERT_EQ(exp2, -50 * 30);
    return true;
}
