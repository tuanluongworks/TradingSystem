cmake_minimum_required(VERSION 3.20)

# Create test executable with just the main file
add_executable(TradingSystemTests test_main.cpp)

# Set include directories for tests
target_include_directories(TradingSystemTests PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Link against modular libs
target_link_libraries(TradingSystemTests PRIVATE serverlib api domain infra persistence)

# Platform-specific libraries for tests
if(WIN32)
    target_link_libraries(TradingSystemTests PRIVATE ws2_32 wsock32)
elseif(UNIX)
    target_link_libraries(TradingSystemTests PRIVATE pthread)
endif()

# Register tests with CTest
add_test(NAME TradingSystemTests COMMAND TradingSystemTests)