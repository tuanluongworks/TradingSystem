# Trading System - Core Application

# Core library (shared between main app and tests)
add_library(trading_core
    # Contracts
    contracts/trading_engine_api.cpp

    # Core models
    core/models/instrument.cpp
    core/models/order.cpp
    core/models/position.cpp
    core/models/trade.cpp
    core/models/market_tick.cpp
    core/models/risk_limit.cpp

    # Core engine
    core/engine/trading_engine.cpp
    core/engine/execution_simulator.cpp

    # Core risk management
    core/risk/risk_manager.cpp

    # Core messaging
    core/messaging/message_queue.cpp

    # Infrastructure
    infrastructure/market_data/websocket_connector.cpp
    infrastructure/market_data/market_data_provider.cpp
    infrastructure/persistence/sqlite_service.cpp

    # UI components
    ui/rendering/opengl_context.cpp
    ui/managers/ui_manager.cpp
    ui/components/market_data_panel.cpp
    ui/components/order_entry_panel.cpp
    ui/components/positions_panel.cpp
    ui/components/trades_panel.cpp
    ui/components/status_panel.cpp

    # Utilities
    utils/logging.cpp
    utils/exceptions.cpp
    utils/config.cpp
)

# Set target properties
target_compile_features(trading_core PUBLIC cxx_std_20)

# Link libraries
target_link_libraries(trading_core
    PUBLIC
        Boost::system
        SQLite::SQLite3
        sqlite_orm::sqlite_orm
        imgui::imgui
        OpenGL::GL
        glfw
        nlohmann_json::nlohmann_json
        spdlog::spdlog
)

# Include directories
target_include_directories(trading_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
)

# Main executable
add_executable(trading_system main.cpp)

target_link_libraries(trading_system
    PRIVATE
        trading_core
)

# Compiler warnings
include(${CMAKE_SOURCE_DIR}/cmake/CompilerWarnings.cmake)
set_project_warnings(trading_core)