cmake_minimum_required(VERSION 3.20)

# Find all test source files
file(GLOB_RECURSE TEST_SOURCES
    test_*.cpp
)

# Create test executable
add_executable(TradingSystemTests ${TEST_SOURCES})

# Link against the library (not the executable)
target_link_libraries(TradingSystemTests TradingSystemLib)

# Platform-specific libraries for tests
if(WIN32)
    target_link_libraries(TradingSystemTests ws2_32 wsock32)
elseif(UNIX)
    target_link_libraries(TradingSystemTests pthread)
endif()

# Register tests with CTest
add_test(NAME TradingSystemTests COMMAND TradingSystemTests)