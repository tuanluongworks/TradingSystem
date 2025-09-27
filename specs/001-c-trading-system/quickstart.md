# Quickstart Guide: C++ Trading System

**Date**: 2025-09-27
**Feature**: C++ Trading System
**Purpose**: Step-by-step validation and setup guide

## Prerequisites

### System Requirements
- **Operating System**: Windows 10+, macOS 11+, or Linux (Ubuntu 20.04+)
- **Compiler**: C++20 compatible (GCC 10+, Clang 12+, MSVC 2019+)
- **Memory**: Minimum 4GB RAM (8GB recommended)
- **Display**: OpenGL 3.3+ support

### Development Tools
- **CMake**: Version 3.20 or higher
- **vcpkg**: For C++ package management
- **Git**: For version control

## Quick Start (5 minutes)

### 1. Clone and Setup
```bash
# Clone the repository
git clone <repository-url>
cd TradingSystem

# Initialize vcpkg (if not already installed)
git clone https://github.com/Microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh  # or bootstrap-vcpkg.bat on Windows

# Set vcpkg integration
./vcpkg/vcpkg integrate install
```

### 2. Install Dependencies
```bash
# Install required packages via vcpkg
./vcpkg/vcpkg install \
    boost-beast \
    sqlite3 \
    sqlite-orm \
    googletest \
    imgui[opengl3-binding,glfw-binding] \
    nlohmann-json \
    spdlog
```

### 3. Build the Application
```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake

# Build the application
cmake --build . --config Release

# Run tests
ctest --output-on-failure
```

### 4. First Run
```bash
# Start the trading application
./bin/trading_system

# Or on Windows
./bin/trading_system.exe
```

## User Acceptance Testing

### Test Scenario 1: Application Startup
**Objective**: Verify the application launches and displays the main interface

**Steps**:
1. Launch the trading application
2. Verify all UI panels are visible:
   - Market Data panel (top-left)
   - Order Entry panel (top-right)
   - Positions panel (bottom-left)
   - Trades panel (bottom-right)
   - Status bar (bottom)
3. Check connection status shows "Disconnected" initially

**Expected Results**:
- Application window opens without errors
- All panels are properly sized and visible
- No crash logs in console
- UI is responsive to mouse interactions

### Test Scenario 2: Market Data Connection
**Objective**: Verify market data simulation connects and displays real-time prices

**Steps**:
1. Click "Connect" in the market data panel
2. Wait for connection status to change to "Connected"
3. Verify default symbols (AAPL, GOOGL, MSFT) appear in market data table
4. Observe price updates occurring every 100-500ms
5. Check that bid/ask spreads are reasonable (ask > bid)

**Expected Results**:
- Connection establishes within 2 seconds
- Status shows "Connected" in green
- Price data updates in real-time
- Spreads are positive and realistic
- No error messages in logs

### Test Scenario 3: Order Entry and Execution
**Objective**: Verify order placement workflow and execution

**Steps**:
1. Ensure market data is connected
2. In Order Entry panel:
   - Select symbol: AAPL
   - Select side: BUY
   - Select type: MARKET
   - Enter quantity: 100
3. Click "Submit Order"
4. Verify order appears in Orders panel with status "NEW"
5. Watch for status change to "FILLED"
6. Check new position appears in Positions panel
7. Verify trade appears in Trades panel

**Expected Results**:
- Order is accepted and assigned unique ID
- Order transitions: NEW → ACCEPTED → FILLED
- Position shows +100 AAPL shares
- Trade record shows execution details
- All updates happen within 1 second

### Test Scenario 4: Risk Management Validation
**Objective**: Verify position limits prevent excessive risk

**Steps**:
1. Place initial order: BUY 100 AAPL (should succeed)
2. Attempt large order: BUY 1000000 AAPL
3. Verify order is rejected with risk message
4. Check order does not appear in working orders
5. Confirm position remains unchanged

**Expected Results**:
- Large order is rejected immediately
- Error message explains position limit exceeded
- No partial execution occurs
- System remains stable and responsive

### Test Scenario 5: Position Tracking and P&L
**Objective**: Verify position calculations and P&L updates

**Steps**:
1. Execute BUY order for 100 shares at market price
2. Note the execution price and position average price
3. Wait for market price to change
4. Verify unrealized P&L updates in real-time
5. Execute partial SELL order for 50 shares
6. Check realized P&L calculation and remaining position

**Expected Results**:
- Position average price matches execution price
- Unrealized P&L = (current_price - avg_price) * quantity
- Realized P&L calculated correctly on partial sale
- Remaining position shows correct quantity (50)

### Test Scenario 6: Data Persistence
**Objective**: Verify trades are saved and retrievable

**Steps**:
1. Execute several trades with different symbols
2. Close the application
3. Restart the application
4. Check that previous positions are restored
5. Verify trade history is preserved
6. Confirm database file exists in expected location

**Expected Results**:
- All positions restored on restart
- Trade history shows all previous executions
- Database file created in ./data/ directory
- No data loss occurs during restart

### Test Scenario 7: Error Handling
**Objective**: Verify graceful handling of error conditions

**Steps**:
1. Disconnect network (if using real data feed)
2. Verify UI shows "Disconnected" status
3. Attempt to place an order
4. Check that order entry is disabled
5. Reconnect network
6. Verify automatic reconnection occurs
7. Test invalid order parameters (negative quantity)

**Expected Results**:
- UI clearly indicates connection loss
- Order entry is disabled during disconnection
- System automatically reconnects when network returns
- Invalid orders are rejected with clear error messages

## Performance Benchmarks

### Latency Requirements
- **Order Execution**: < 1ms from submit to acknowledgment
- **Market Data Updates**: < 100ms from tick to UI display
- **UI Responsiveness**: 60 FPS render rate maintained
- **Database Writes**: < 10ms per trade record

### Load Testing
```bash
# Run performance tests
./bin/performance_tests

# Expected results:
# - Handle 1000 orders/second without degradation
# - Process 10000 market ticks/second
# - Maintain < 100MB memory usage
# - Zero memory leaks detected
```

## Configuration

### Application Settings
Default configuration file: `config/trading_system.json`

```json
{
  "market_data": {
    "simulation_mode": true,
    "websocket_url": "wss://api.example.com/v1/market_data",
    "symbols": ["AAPL", "GOOGL", "MSFT", "TSLA", "AMZN"],
    "update_interval_ms": 100
  },
  "risk_management": {
    "max_position_size": 10000,
    "max_order_size": 1000,
    "max_daily_loss": 50000
  },
  "ui": {
    "theme": "dark",
    "auto_sort": true,
    "precision": 2,
    "refresh_rate_ms": 100
  },
  "persistence": {
    "database_path": "./data/trading.db",
    "backup_path": "./data/backups/",
    "auto_backup": true
  }
}
```

## Troubleshooting

### Common Issues

**Build Errors**:
- Ensure vcpkg is properly integrated
- Verify C++20 compiler support
- Check CMake version >= 3.20

**Runtime Crashes**:
- Verify OpenGL drivers are updated
- Check console logs for error details
- Ensure required directories exist (./data/, ./logs/)

**Performance Issues**:
- Monitor CPU usage during market data updates
- Check memory usage for leaks
- Verify database file permissions

### Debug Mode
```bash
# Build in debug mode for detailed logging
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .

# Run with verbose logging
./bin/trading_system --log-level=debug
```

## Next Steps

After successful quickstart validation:

1. **Customize Configuration**: Modify settings in `config/trading_system.json`
2. **Add Instruments**: Configure additional symbols for trading
3. **Tune Risk Limits**: Adjust position and order size limits
4. **Integration**: Connect to real market data feeds (requires API keys)
5. **Monitoring**: Set up logging and monitoring for production use

## Support

- **Documentation**: See `docs/` directory for detailed guides
- **API Reference**: Generated from contracts in `specs/*/contracts/`
- **Performance Tuning**: See `docs/performance.md`
- **Deployment**: See `docs/deployment.md`