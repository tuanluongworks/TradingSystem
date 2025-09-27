# Tasks: C++ Trading System

**Input**: Design documents from `/Users/tuanluong/Documents/GithubProjects/TradingSystem/specs/001-c-trading-system/`
**Prerequisites**: plan.md, research.md, data-model.md, contracts/

## Execution Flow (main)
```
1. Load plan.md from feature directory
   → Found: C++20 with Dear ImGui, OpenGL, Boost.Beast, sqlite_orm, GoogleTest
   → Extract: Three-tier architecture, thread-safe messaging, TDD approach
2. Load design documents:
   → data-model.md: 6 core entities (Instrument, Order, Position, Trade, MarketTick, RiskLimit)
   → contracts/: 2 API contracts (UI Interface, Trading Engine API)
   → research.md: Tech stack decisions and best practices
3. Generate tasks by category following constitutional requirements
4. Apply TDD: Tests before implementation, [P] for parallel execution
5. Number tasks sequentially (T001-T042)
6. Validate completeness and dependencies
```

## Format: `[ID] [P?] Description`
- **[P]**: Can run in parallel (different files, no dependencies)
- Include exact file paths in descriptions

## Phase 3.1: Project Setup ✅ COMPLETED

- [X] T001 Create CMake build system with vcpkg integration in /CMakeLists.txt and /cmake/
- [X] T002 Initialize C++20 project structure following plan.md architecture in /src/, /tests/, /include/
- [X] T003 [P] Configure Google C++ Style Guide linting with clang-format in /.clang-format
- [X] T004 [P] Setup GoogleTest framework integration in /tests/CMakeLists.txt
- [X] T005 [P] Configure vcpkg.json with all required dependencies (Dear ImGui, Boost.Beast, sqlite_orm, etc.)

## Phase 3.2: Contract Tests First (TDD) ✅ COMPLETED
**Tests written and initially fail as required by TDD**

- [X] T006 [P] UI Manager interface contract test in /tests/unit/ui/test_ui_manager_interface.cpp
- [X] T007 [P] Market Data Panel interface contract test in /tests/unit/ui/test_market_data_panel_interface.cpp
- [X] T008 [P] Order Entry Panel interface contract test in /tests/unit/ui/test_order_entry_panel_interface.cpp
- [X] T009 [P] Positions Panel interface contract test in /tests/unit/ui/test_positions_panel_interface.cpp
- [X] T010 [P] Trading Engine API contract test in /tests/unit/core/test_trading_engine_interface.cpp
- [X] T011 [P] Market Data Provider interface contract test in /tests/unit/infrastructure/test_market_data_provider_interface.cpp
- [X] T012 [P] Risk Manager interface contract test in /tests/unit/core/test_risk_manager_interface.cpp
- [X] T013 [P] Persistence Service interface contract test in /tests/unit/infrastructure/test_persistence_service_interface.cpp

## Phase 3.3: Core Entity Models ✅ COMPLETED

- [X] T014 [P] Instrument model with validation in /src/core/models/instrument.hpp and /src/core/models/instrument.cpp
- [X] T015 [P] Order model with state machine in /src/core/models/order.hpp and /src/core/models/order.cpp
- [X] T016 [P] Position model with P&L calculations in /src/core/models/position.hpp and /src/core/models/position.cpp
- [X] T017 [P] Trade model with execution details in /src/core/models/trade.hpp and /src/core/models/trade.cpp
- [X] T018 [P] MarketTick data structure in /src/core/models/market_tick.hpp and /src/core/models/market_tick.cpp
- [X] T019 [P] RiskLimit model with validation logic in /src/core/models/risk_limit.hpp and /src/core/models/risk_limit.cpp

## Phase 3.4: Infrastructure Layer ✅ PARTIALLY COMPLETED

- [X] T020 Thread-safe MessageQueue template implementation in /src/core/messaging/message_queue.hpp
- [X] T021 Exception handling and logging utilities in /src/utils/logging.hpp and /src/utils/exceptions.hpp
- [X] T022 WebSocket market data connector with Boost.Beast in /src/infrastructure/market_data/websocket_connector.hpp and .cpp
- [X] T023 SQLite persistence service with sqlite_orm in /src/infrastructure/persistence/sqlite_service.hpp and .cpp
- [X] T024 Configuration management system in /src/utils/config.hpp and /src/utils/config.cpp

## Phase 3.5: Core Trading Engine

- [X] T025 Risk Manager implementation with pre-trade validation in /src/core/risk/risk_manager.hpp and .cpp
- [ ] T026 Trading Engine with order lifecycle management in /src/core/engine/trading_engine.hpp and .cpp
- [ ] T027 Market Data Provider with subscription management in /src/infrastructure/market_data/market_data_provider.hpp and .cpp
- [ ] T028 Order execution simulator for development mode in /src/core/engine/execution_simulator.hpp and .cpp

## Phase 3.6: UI Layer Implementation

- [ ] T029 OpenGL rendering context setup with Dear ImGui in /src/ui/rendering/opengl_context.hpp and .cpp
- [ ] T030 UI Manager main application loop in /src/ui/managers/ui_manager.hpp and .cpp
- [ ] T031 Market Data Panel ImGui implementation in /src/ui/components/market_data_panel.hpp and .cpp
- [ ] T032 Order Entry Panel with form validation in /src/ui/components/order_entry_panel.hpp and .cpp
- [ ] T033 Positions Panel with P&L display in /src/ui/components/positions_panel.hpp and .cpp
- [ ] T034 Trades Panel with execution history in /src/ui/components/trades_panel.hpp and .cpp
- [ ] T035 Status Panel with connection indicators in /src/ui/components/status_panel.hpp and .cpp

## Phase 3.7: Integration Tests

- [ ] T036 [P] End-to-end order submission test in /tests/integration/test_order_lifecycle.cpp
- [ ] T037 [P] Market data flow integration test in /tests/integration/test_market_data_flow.cpp
- [ ] T038 [P] Position tracking integration test in /tests/integration/test_position_tracking.cpp
- [ ] T039 [P] Risk management integration test in /tests/integration/test_risk_validation.cpp
- [ ] T040 [P] Database persistence integration test in /tests/integration/test_data_persistence.cpp

## Phase 3.8: Performance and Polish

- [ ] T041 Performance benchmarks for latency-critical paths in /tests/performance/test_order_latency.cpp
- [ ] T042 Main application entry point with initialization in /src/main.cpp

## Dependencies

**Setup Dependencies**:
- T001-T005 must complete before any other tasks

**Contract Test Dependencies**:
- T006-T013 (contract tests) must complete and FAIL before T014-T042
- All contract tests are parallel [P] as they test different interfaces

**Model Dependencies**:
- T014-T019 (models) are parallel [P] as they create independent entities
- T020-T024 (infrastructure) depend on T014-T019 for model definitions

**Engine Dependencies**:
- T025 (Risk Manager) depends on T014-T019 (models) and T020 (messaging)
- T026 (Trading Engine) depends on T025 (Risk Manager) and T014-T019 (models)
- T027 (Market Data Provider) depends on T018 (MarketTick) and T020 (messaging)
- T028 (Execution Simulator) depends on T026 (Trading Engine)

**UI Dependencies**:
- T029 (OpenGL context) can run after T005 (project setup)
- T030-T035 (UI components) depend on T029 and T014-T019 (models)

**Integration Dependencies**:
- T036-T040 (integration tests) depend on all implementation tasks T025-T035
- Integration tests are parallel [P] as they test different workflows

**Final Dependencies**:
- T041 (performance tests) depends on T036-T040 (integration complete)
- T042 (main.cpp) depends on T030 (UI Manager) and T026 (Trading Engine)

## Parallel Execution Examples

### Phase 3.2 - Contract Tests (All Parallel)
```bash
# Launch all contract tests together:
Task: "UI Manager interface contract test in /tests/unit/ui/test_ui_manager_interface.cpp"
Task: "Trading Engine API contract test in /tests/unit/core/test_trading_engine_interface.cpp"
Task: "Market Data Provider interface contract test in /tests/unit/infrastructure/test_market_data_provider_interface.cpp"
Task: "Risk Manager interface contract test in /tests/unit/core/test_risk_manager_interface.cpp"
# ... (all T006-T013 in parallel)
```

### Phase 3.3 - Core Models (All Parallel)
```bash
# Launch all model implementations together:
Task: "Instrument model with validation in /src/core/models/instrument.hpp and .cpp"
Task: "Order model with state machine in /src/core/models/order.hpp and .cpp"
Task: "Position model with P&L calculations in /src/core/models/position.hpp and .cpp"
Task: "Trade model with execution details in /src/core/models/trade.hpp and .cpp"
Task: "MarketTick data structure in /src/core/models/market_tick.hpp and .cpp"
Task: "RiskLimit model with validation logic in /src/core/models/risk_limit.hpp and .cpp"
```

### Phase 3.7 - Integration Tests (All Parallel)
```bash
# Launch all integration tests together:
Task: "End-to-end order submission test in /tests/integration/test_order_lifecycle.cpp"
Task: "Market data flow integration test in /tests/integration/test_market_data_flow.cpp"
Task: "Position tracking integration test in /tests/integration/test_position_tracking.cpp"
Task: "Risk management integration test in /tests/integration/test_risk_validation.cpp"
Task: "Database persistence integration test in /tests/integration/test_data_persistence.cpp"
```

## Constitutional Compliance

**Performance First**: ✅
- T041 includes latency benchmarks (<1ms order execution)
- T020 implements lock-free message queues for high performance
- T025-T028 focus on optimized trading engine implementation

**Robustness and Reliability**: ✅
- T021 establishes comprehensive exception handling
- T023 implements ACID-compliant SQLite persistence
- T039 validates risk management prevents invalid trades

**Modularity and Separation of Concerns**: ✅
- Clear separation: T014-T019 (models), T025-T028 (engine), T029-T035 (UI)
- T020 provides thread-safe communication between layers
- Each component has isolated testing (T006-T013)

**Testing Excellence**: ✅
- TDD enforced: Contract tests (T006-T013) before implementation
- Comprehensive coverage: Unit, integration, and performance tests
- GoogleTest framework mandated throughout

**Code Quality and Standards**: ✅
- T003 enforces Google C++ Style Guide with clang-format
- C++20 standard enforced throughout all tasks
- Smart pointers and RAII patterns required

## Validation Checklist
*GATE: Checked by implementation before execution*

- [x] All contracts have corresponding tests (T006-T013 cover all interfaces)
- [x] All entities have model tasks (T014-T019 cover all 6 entities from data-model.md)
- [x] All tests come before implementation (Phase 3.2 before 3.3-3.8)
- [x] Parallel tasks truly independent ([P] tasks operate on different files)
- [x] Each task specifies exact file path (absolute paths included)
- [x] No task modifies same file as another [P] task (verified file isolation)
- [x] Constitutional requirements addressed in task design
- [x] Dependencies properly ordered for build success

## Notes

- **[P] tasks** = different files, no dependencies, can run in parallel
- **TDD enforced**: Verify contract tests fail before implementing interfaces
- **Thread safety**: All cross-layer communication via MessageQueue (T020)
- **Performance critical**: Order execution path optimized in T025-T028
- **UI responsiveness**: Async message processing prevents blocking (T030-T035)
- **Data integrity**: SQLite ACID compliance + backup logging (T023)
- **Risk management**: Pre-trade validation prevents invalid orders (T025)