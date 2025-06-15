cmake_minimum_required(VERSION 3.20)
project(TradingSystem)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set Visual Studio 2022 toolset
if(MSVC)
    set(CMAKE_GENERATOR_TOOLSET "v143")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options(/EHsc) # Enable exception handling
    add_compile_definitions(_WIN32_WINNT=0x0A00)
    add_compile_options(/wd4996) # Disable unsafe function warnings
endif()

# Cross-platform compiler flags
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wall -Wextra -pedantic)
endif()

include_directories(src)
include_directories(src/common)
include_directories(src/trading)
include_directories(src/database)

# Collect all source files except main.cpp
file(GLOB_RECURSE LIB_SOURCES
    src/server/*.cpp
    src/trading/*.cpp
    src/api/*.cpp
    src/database/*.cpp
    src/utils/*.cpp
)

# Create a static library for the core functionality
add_library(TradingSystemLib STATIC ${LIB_SOURCES})

# Create the main executable
add_executable(TradingSystem src/main.cpp)
target_link_libraries(TradingSystem TradingSystemLib)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(TradingSystem ws2_32 wsock32)
    target_link_libraries(TradingSystemLib ws2_32 wsock32)
elseif(UNIX)
    target_link_libraries(TradingSystem pthread)
    target_link_libraries(TradingSystemLib pthread)
endif()

enable_testing()
add_subdirectory(tests)