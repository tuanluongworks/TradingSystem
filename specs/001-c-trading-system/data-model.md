# Data Model: C++ Trading System

**Date**: 2025-09-27
**Feature**: C++ Trading System
**Scope**: Core entities, relationships, and state transitions

## Core Entities

### Instrument
Represents a tradeable financial security.

```cpp
class Instrument {
public:
    std::string symbol;           // Unique identifier (e.g., "AAPL", "EURUSD")
    std::string name;             // Human-readable name
    InstrumentType type;          // STOCK, FOREX, CRYPTO, etc.
    double tick_size;             // Minimum price increment
    int lot_size;                 // Minimum quantity increment
    bool is_active;               // Trading enabled flag

    // Market data (mutable, updated real-time)
    mutable double bid_price;     // Current bid price
    mutable double ask_price;     // Current ask price
    mutable double last_price;    // Last traded price
    mutable std::chrono::system_clock::time_point last_update;
};

enum class InstrumentType {
    STOCK,
    FOREX,
    CRYPTO,
    COMMODITY,
    INDEX
};
```

**Relationships**:
- One-to-many with Orders
- One-to-many with Positions
- One-to-many with Trades

**Validation Rules**:
- Symbol must be unique and non-empty
- Prices must be positive
- Tick size and lot size must be positive
- Last update timestamp must be recent for active instruments

### Order
Represents a trading instruction with complete lifecycle tracking.

```cpp
class Order {
public:
    std::string order_id;         // Unique system-generated ID
    std::string instrument_symbol; // Reference to Instrument
    OrderSide side;               // BUY or SELL
    OrderType type;               // MARKET or LIMIT
    double quantity;              // Requested quantity
    double price;                 // Limit price (0 for market orders)
    OrderStatus status;           // Current order state
    double filled_quantity;       // Quantity already executed
    double remaining_quantity;    // Quantity still working
    std::chrono::system_clock::time_point created_time;
    std::chrono::system_clock::time_point last_modified;
    std::string rejection_reason; // If status == REJECTED

    // Calculated fields
    double get_average_fill_price() const;
    bool is_fully_filled() const;
    bool is_working() const;
};

enum class OrderSide {
    BUY,
    SELL
};

enum class OrderType {
    MARKET,
    LIMIT
};

enum class OrderStatus {
    NEW,              // Just created, not yet sent
    ACCEPTED,         // Accepted by execution engine
    PARTIALLY_FILLED, // Some quantity executed
    FILLED,           // Completely executed
    CANCELED,         // Canceled by user
    REJECTED          // Rejected by risk management or engine
};
```

**State Transitions**:
```
NEW → ACCEPTED → PARTIALLY_FILLED → FILLED
  ↓       ↓            ↓
REJECTED CANCELED   CANCELED
```

**Relationships**:
- Many-to-one with Instrument
- One-to-many with Trades (execution reports)
- Affects Position calculations

**Validation Rules**:
- Order ID must be unique across system
- Quantity must be positive
- Price must be positive for LIMIT orders
- Instrument symbol must reference valid active instrument
- Status transitions must follow state machine rules

### Position
Represents current holdings in an instrument.

```cpp
class Position {
public:
    std::string instrument_symbol; // Reference to Instrument
    double quantity;               // Net position (positive=long, negative=short)
    double average_price;          // Volume-weighted average price
    double realized_pnl;           // Profit/loss from closed trades
    double unrealized_pnl;         // Current mark-to-market P&L
    std::chrono::system_clock::time_point last_updated;

    // Calculated fields
    double get_market_value(double current_price) const;
    double get_total_pnl(double current_price) const;
    bool is_flat() const;           // quantity == 0
};
```

**Relationships**:
- Many-to-one with Instrument
- Updated by Trade executions
- Used by RiskManager for limit validation

**Validation Rules**:
- Each instrument can have at most one Position
- Average price must be positive for non-zero positions
- Position updates must maintain P&L consistency

### Trade
Represents an executed transaction (execution report).

```cpp
class Trade {
public:
    std::string trade_id;         // Unique system-generated ID
    std::string order_id;         // Reference to parent Order
    std::string instrument_symbol; // Reference to Instrument
    OrderSide side;               // BUY or SELL (copied from Order)
    double quantity;              // Executed quantity
    double price;                 // Execution price
    std::chrono::system_clock::time_point execution_time;
    TradeType type;               // FULL or PARTIAL fill

    // Calculated fields
    double get_notional_value() const; // quantity * price
    double get_commission() const;      // Based on trade size
};

enum class TradeType {
    FULL_FILL,    // Order completely executed
    PARTIAL_FILL  // Order partially executed
};
```

**Relationships**:
- Many-to-one with Order
- Many-to-one with Instrument
- Updates Position calculations
- Creates audit trail

**Validation Rules**:
- Trade ID must be unique
- Quantity and price must be positive
- Order ID must reference existing Order
- Execution time must be reasonable (not future, not too old)

### Market Data Tick
Represents real-time price updates.

```cpp
struct MarketTick {
    std::string instrument_symbol;
    double bid_price;
    double ask_price;
    double last_price;
    double volume;                // Trade volume
    std::chrono::system_clock::time_point timestamp;

    // Validation
    bool is_valid() const;
    double get_spread() const;    // ask - bid
    double get_mid_price() const; // (bid + ask) / 2
};
```

**Relationships**:
- Updates Instrument market data
- Triggers Position P&L recalculation
- Used for order execution in simulation mode

**Validation Rules**:
- All prices must be positive
- Ask price must be >= bid price
- Timestamp must be recent
- Volume must be non-negative

### Risk Limit
Represents position and risk constraints.

```cpp
class RiskLimit {
public:
    std::string instrument_symbol; // Reference to Instrument (empty = global)
    LimitType type;               // POSITION_LIMIT, ORDER_SIZE_LIMIT, etc.
    double max_value;             // Maximum allowed value
    bool is_active;               // Enable/disable limit

    // Validation method
    bool check_order(const Order& order, const Position* current_position) const;
};

enum class LimitType {
    MAX_POSITION_SIZE,    // Maximum net position quantity
    MAX_ORDER_SIZE,       // Maximum single order quantity
    MAX_DAILY_VOLUME,     // Maximum daily trading volume
    MAX_LOSS_LIMIT        // Maximum daily loss limit
};
```

**Relationships**:
- May reference specific Instrument (or global)
- Used by RiskManager for pre-trade validation
- Applied to Orders before execution

**Validation Rules**:
- Max value must be positive
- Instrument-specific limits override global limits
- At least one limit type must be configured per instrument

## Entity Relationships

### Primary Relationships
1. **Instrument ↔ Order**: One instrument can have many orders
2. **Instrument ↔ Position**: One instrument has at most one position
3. **Order ↔ Trade**: One order can generate multiple trades (partial fills)
4. **Trade → Position**: Trades update position calculations
5. **Instrument ↔ RiskLimit**: Instruments have associated risk limits

### Data Flow
1. **Market Data Flow**: MarketTick → Instrument → Position P&L update
2. **Order Flow**: Order → RiskManager → ExecutionEngine → Trade → Position
3. **UI Update Flow**: Position/Order changes → UI notification queue

## Aggregate Operations

### Portfolio Summary
Calculated aggregate showing total portfolio metrics:
- Total unrealized P&L across all positions
- Total realized P&L for the day
- Total market value of all positions
- Count of active orders by status

### Daily Trading Summary
Calculated aggregate for reporting:
- Total volume traded by instrument
- Number of trades executed
- Average order size
- P&L by instrument

## Persistence Strategy

### Database Tables
- instruments (symbol, name, type, tick_size, lot_size, is_active)
- orders (order_id, symbol, side, type, quantity, price, status, created_time, etc.)
- trades (trade_id, order_id, symbol, side, quantity, price, execution_time)
- positions (symbol, quantity, average_price, realized_pnl, last_updated)
- risk_limits (symbol, type, max_value, is_active)

### Backup Logging
- All trades written to daily CSV files for disaster recovery
- Order status changes logged to audit trail
- Market data snapshots for replay capability

## Thread Safety Considerations

### Concurrent Access Patterns
- Market data updates: Single writer (market data thread), multiple readers
- Order operations: Multiple writers (UI + engine), multiple readers
- Position calculations: Single writer (engine thread), multiple readers
- Risk checks: Multiple readers, infrequent writers (configuration updates)

### Synchronization Strategy
- Read-write locks for market data and positions
- Mutex protection for order state transitions
- Lock-free queues for high-frequency message passing
- Atomic operations for simple numeric updates

## Validation and Constraints

### Cross-Entity Validation
- Order quantity must respect instrument lot size
- Order price must align with instrument tick size
- Position updates must maintain P&L consistency
- Risk limit violations must prevent order acceptance

### Business Logic Constraints
- Cannot sell more than current long position (no short selling)
- Orders must reference active instruments only
- Price precision must match instrument specifications
- All monetary calculations use fixed-point arithmetic for precision