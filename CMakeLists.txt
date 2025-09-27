cmake_minimum_required(VERSION 3.20)
project(TradingSystem VERSION 1.0.0 LANGUAGES CXX)

# C++20 standard required
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Configure vcpkg
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

# Build type setup
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4 /WX)
    add_compile_definitions(_WIN32_WINNT=0x0601)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-Wno-psabi)
    endif()
endif()

# Find packages
find_package(Boost REQUIRED COMPONENTS system)
find_package(SQLite3 REQUIRED)
find_package(sqlite_orm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# Enable testing
enable_testing()
find_package(GTest CONFIG REQUIRED)

# Include directories
include_directories(include)
include_directories(src)

# Add subdirectories
add_subdirectory(src)
add_subdirectory(tests)

# Add cmake utilities
add_subdirectory(cmake)

# Installation
install(TARGETS trading_system DESTINATION bin)
install(DIRECTORY config/ DESTINATION config)
install(DIRECTORY docs/ DESTINATION docs)