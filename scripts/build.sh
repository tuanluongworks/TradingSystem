#!/bin/bash

# Navigate to the project directory
cd "$(dirname "$0")/.."

# Create a build directory if it doesn't exist
mkdir -p build
cd build

# Run CMake to configure the project
cmake ..

# Build the project
cmake --build .

# Optionally, run tests after building
# ctest --output-on-failure