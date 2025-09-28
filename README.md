# Trading System

C++20 desktop trading terminal that combines a high-performance core engine with an immediate-mode UI. The application simulates a complete trading workflow end-to-end: market data ingestion, order lifecycle management, risk control, persistence, and real-time visualization.

## Highlights
- Event-driven trading engine with pluggable execution simulator
- Market data provider supporting simulated WebSocket feeds and symbol subscriptions
- Pre-trade risk checks with configurable position and order limits
- SQLite-backed persistence layer with automatic backups and CSV export hooks
- ImGui + OpenGL/GLFW UI composed of modular panels (market data, order entry, positions, trades, status)
- Structured logging via spdlog with console and rotating file sinks
- Comprehensive automated test suite (unit, integration, performance) built on GoogleTest

## Architecture Sketch
- **Core (`src/core`)**: domain models, execution simulator, trading engine, risk manager, message queues
- **Infrastructure (`src/infrastructure`)**: market data connectors, persistence services (SQLite + sqlite_orm)
- **UI (`src/ui`)**: rendering context, panel managers, ImGui components
- **Utilities (`src/utils`)**: configuration manager, logging helpers, shared exception types
- **Contracts (`src/contracts`)**: interface boundaries consumed by tests and future adapters

Supporting assets live under `include/` (public headers), `config/` (runtime settings), `data/` (runtime artifacts), and `specs/` (product documentation and acceptance guides).

## Repository Layout
```
├── CMakeLists.txt          # Root build orchestration
├── cmake/                  # Build utilities (warnings, toolchain helpers)
├── config/trading_system.json
├── include/                # Public headers shared by app and tests
├── src/                    # Application sources (core, infrastructure, UI, utils)
├── tests/                  # Unit, integration, performance test targets
├── data/                   # Runtime database/backups (created at runtime)
└── logs/                   # Rotating log files (created at runtime)
```

## Prerequisites
- C++20 toolchain (GCC ≥ 10, Clang ≥ 12, MSVC ≥ 2019)
- CMake ≥ 3.20
- vcpkg (or alternative package manager) with integration enabled
- OpenGL 3.3-compatible GPU/driver

The project depends on the following libraries (resolved via vcpkg by default):
`boost-system`, `sqlite3`, `sqlite-orm`, `nlohmann-json`, `spdlog`, `imgui[opengl3-binding,glfw-binding]`, `glfw3`, `gtest`.

## Build & Run
1. Clone and initialize dependencies
   ```bash
   git clone <repository-url>
   cd TradingSystem
   git submodule update --init --recursive  # if applicable
   ```
2. Bootstrap vcpkg (skip if already installed locally)
   ```bash
   git clone https://github.com/microsoft/vcpkg.git
   ./vcpkg/bootstrap-vcpkg.sh        # or .\vcpkg\bootstrap-vcpkg.bat on Windows
   ./vcpkg/vcpkg integrate install
   ```
3. Install required packages (one-time)
   ```bash
   ./vcpkg/vcpkg install \
       boost-system \
       sqlite3 \
       sqlite-orm \
       nlohmann-json \
       spdlog \
       imgui[opengl3-binding,glfw-binding] \
       glfw3 \
       gtest
   ```
4. Configure the build
   ```bash
   cmake -S . -B build \
     -DCMAKE_BUILD_TYPE=Release \
     -DCMAKE_TOOLCHAIN_FILE="$(pwd)/vcpkg/scripts/buildsystems/vcpkg.cmake"
   ```
5. Compile targets
   ```bash
   cmake --build build --config Release
   ```
6. Launch the trading application
   ```bash
   ./build/trading_system            # macOS/Linux
   ./build/Release/trading_system.exe  # Windows multi-config generators
   ```

### Debug Builds
Use `-DCMAKE_BUILD_TYPE=Debug` (single-config) or pass `--config Debug` when invoking the build and executable.

## Testing
Three dedicated binaries are generated under `build/`:
- `unit_tests` – validates contracts and core components
- `integration_tests` – exercises cross-component workflows (order lifecycle, data flow)
- `performance_tests` – synthetic load benchmarks

Typical workflows:
```bash
# Discover and run all registered tests via CTest
ctest --test-dir build --output-on-failure

# Or execute a specific suite
./build/unit_tests
./build/integration_tests
./build/performance_tests
```

## Runtime Configuration
All runtime knobs live in `config/trading_system.json`. Key sections:
- `market_data`: simulation toggle, WebSocket endpoint, subscribed symbols, update cadence
- `risk_management`: position/order limits, daily loss guardrails, per-symbol overrides
- `ui`: theming, refresh cadence, panel visibility, row caps
- `persistence`: SQLite paths, backup cadence, CSV export options
- `logging`: log levels, sink destinations, rotation settings

The configuration manager validates inputs on startup and supports runtime reloads through API calls. Default data/log directories are relative to the executable; ensure the process can create `./data/` and `./logs/`.

## Operational Notes
- Market data starts in simulation mode; integrate a live feed by swapping the connector implementation and updating configuration.
- Order execution currently uses an in-memory simulator that produces fills and partial fills. Replace with real broker adapters via the contracts in `src/contracts/`.
- Persistence leverages SQLite with sqlite_orm for schema management and supports offline backup and export hooks.
- Logging defaults to `info` level; increase to `debug` for verbose diagnostics during development.

## Development Workflow
- Use `cmake/CompilerWarnings.cmake` to enforce consistent warnings across targets (already included by default).
- Follow the existing directory conventions when adding new features (core logic in `src/core`, adapters in `src/infrastructure`, UI widgets in `src/ui/components`).
- Add unit tests for new components and expand integration scenarios when modifying cross-cutting flows.
- Keep configuration surface documented; update `README.md` and `specs/quickstart.md` when introducing new settings or modules.
