# Trading System - Tests

# Unit tests
add_executable(unit_tests
    # Core model tests
    unit/core/test_trading_engine_interface.cpp
    unit/core/test_risk_manager_interface.cpp

    # Infrastructure tests
    unit/infrastructure/test_market_data_provider_interface.cpp
    unit/infrastructure/test_persistence_service_interface.cpp

    # UI tests
    unit/ui/test_ui_manager_interface.cpp
    unit/ui/test_market_data_panel_interface.cpp
    unit/ui/test_order_entry_panel_interface.cpp
    unit/ui/test_positions_panel_interface.cpp
)

target_link_libraries(unit_tests
    PRIVATE
        trading_core
        GTest::gtest_main
        GTest::gmock_main
)

# Integration tests
add_executable(integration_tests
    integration/test_order_lifecycle.cpp
    integration/test_market_data_flow.cpp
    integration/test_position_tracking.cpp
    integration/test_risk_validation.cpp
    integration/test_data_persistence.cpp
)

target_link_libraries(integration_tests
    PRIVATE
        trading_core
        GTest::gtest_main
        GTest::gmock_main
)

# Performance tests
add_executable(performance_tests
    performance/test_order_latency.cpp
)

target_link_libraries(performance_tests
    PRIVATE
        trading_core
        GTest::gtest_main
        GTest::gmock_main
)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(unit_tests)
gtest_discover_tests(integration_tests)
gtest_discover_tests(performance_tests)