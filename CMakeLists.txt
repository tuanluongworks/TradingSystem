project(TTradingSystem)

set(CMAKE_CXX_STANDARD 17)

include_directories(src)

file(GLOB_RECURSE SOURCES
    src/*.cpp
)

add_executable(TradingSystem ${SOURCES})

enable_testing()

add_subdirectory(tests)