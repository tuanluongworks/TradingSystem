@echo off
rem This script runs the tests for the TradingSystem project on Windows.

setlocal

rem Define the build directory
set BUILD_DIR=build

rem Check if build directory exists
if not exist %BUILD_DIR% (
    echo Build directory not found. Please build the project first using build.bat
    exit /b 1
)

echo Running Trading System Tests...
echo ================================

rem Run unit tests
if exist build\Release\TradingSystemTests.exe (
    echo Running unit tests...
    build\Release\TradingSystemTests.exe
    if %ERRORLEVEL% neq 0 (
        echo Unit tests failed!
        cd ..
        exit /b 1
    )
) else (
    echo Warning: TradingSystemTests.exe not found. Skipping unit tests.
)

rem Run integration tests if they exist
if exist build\Release\IntegrationTests.exe (
    echo.
    echo Running integration tests...
    build\Release\IntegrationTests.exe
    if %ERRORLEVEL% neq 0 (
        echo Integration tests failed!
        cd ..
        exit /b 1
    )
)

cd ..

echo.
echo All tests completed successfully!
endlocal 