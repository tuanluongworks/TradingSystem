@echo off
rem This script builds the TradingSystem project using CMake.

setlocal

rem Define the build directory
set BUILD_DIR=build

rem Clean previous build to avoid issues
if exist %BUILD_DIR% (
    rmdir /s /q %BUILD_DIR%
)

rem Create the build directory
mkdir %BUILD_DIR%

cd %BUILD_DIR%

rem Run CMake to configure the project with proper warning level
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_CXX_FLAGS="/W3"

if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed!
    cd ..
    exit /b 1
)

rem Build the project
cmake --build . --config Release

if %ERRORLEVEL% neq 0 (
    echo Build failed!
    cd ..
    exit /b 1
)

endlocal
echo Build completed successfully.