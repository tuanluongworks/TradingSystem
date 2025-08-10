cmake_minimum_required(VERSION 3.20)
project(TradingSystem)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_UNITY "Enable unity builds" OFF)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)

# Set Visual Studio 2022 toolset
if(MSVC)
    set(CMAKE_GENERATOR_TOOLSET "v143")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options(/EHsc /permissive- /W4)
else()
    # Cross-platform compiler flags
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion)
endif()

if(ENABLE_ASAN AND NOT MSVC)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()
if(ENABLE_TSAN AND NOT MSVC)
    add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
    add_link_options(-fsanitize=thread)
endif()

include(GNUInstallDirs)

# Include directories
include_directories(src)
include_directories(src/common)
include_directories(src/trading)
include_directories(src/database)

# Source files
file(GLOB_RECURSE SRC_SERVER src/server/*.cpp)
file(GLOB_RECURSE SRC_TRADING src/trading/*.cpp)
file(GLOB_RECURSE SRC_API src/api/*.cpp)
file(GLOB_RECURSE SRC_DB src/database/*.cpp)
file(GLOB_RECURSE SRC_UTILS src/utils/*.cpp)
file(GLOB_RECURSE SRC_INFRA src/infrastructure/*.cpp)

# Set sources for different components
set(DOMAIN_SOURCES ${SRC_TRADING})
set(API_SOURCES ${SRC_API})
set(DB_SOURCES ${SRC_DB})
set(INFRA_SOURCES ${SRC_INFRA} ${SRC_UTILS})
set(SERVER_SOURCES ${SRC_SERVER})

# Create component libraries
add_library(domain STATIC ${DOMAIN_SOURCES})
add_library(api STATIC ${API_SOURCES})
add_library(persistence STATIC ${DB_SOURCES})
add_library(infra STATIC ${INFRA_SOURCES})
add_library(serverlib STATIC ${SERVER_SOURCES})

# Enable unity builds if the option is set
if(ENABLE_UNITY)
    set_target_properties(domain api persistence infra serverlib PROPERTIES UNITY_BUILD ON)
endif()

# Create the main executable
add_executable(app_server src/main.cpp)

# Link layering: app_server -> serverlib -> api -> domain -> persistence + infra
# and infra for cross-cutting utilities
set_target_properties(domain api persistence infra serverlib PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES)

# Public includes
foreach(tgt IN ITEMS domain api persistence infra serverlib)
    target_include_directories(${tgt} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
endforeach()

# Link libraries
target_link_libraries(serverlib PUBLIC api domain infra persistence)
target_link_libraries(app_server PRIVATE serverlib)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(app_server ws2_32 wsock32)
    target_link_libraries(serverlib ws2_32 wsock32)
elseif(UNIX)
    target_link_libraries(app_server pthread)
    target_link_libraries(serverlib pthread)
endif()

enable_testing()
add_subdirectory(tests)