# Trading System

## Overview
The Trading System is a C++ application designed to facilitate trading operations, including order management, portfolio tracking, and strategy implementation. This project serves as a demonstration of trading application development and showcases various components involved in building a trading system.

## Project Structure
```
TradingSystem
├── src
│   ├── main.cpp
│   ├── core
│   │   ├── TradingEngine.cpp
│   │   ├── TradingEngine.h
│   │   ├── OrderManager.cpp
│   │   ├── OrderManager.h
│   │   ├── PortfolioManager.cpp
│   │   └── PortfolioManager.h
│   ├── models
│   │   ├── Order.h
│   │   ├── Trade.h
│   │   ├── Portfolio.h
│   │   └── MarketData.h
│   ├── strategies
│   │   ├── Strategy.h
│   │   ├── MovingAverageStrategy.cpp
│   │   └── MovingAverageStrategy.h
│   ├── data
│   │   ├── DataProvider.h
│   │   ├── MarketDataProvider.cpp
│   │   └── MarketDataProvider.h
│   └── utils
│       ├── Logger.cpp
│       ├── Logger.h
│       ├── Config.cpp
│       └── Config.h
├── tests
│   ├── test_main.cpp
│   ├── TradingEngineTest.cpp
│   └── OrderManagerTest.cpp
├── CMakeLists.txt
├── .gitignore
└── README.md
```

## Features
- **Trading Engine**: Manages the overall trading process, including order execution and strategy application.
- **Order Management**: Handles order creation, modification, and cancellation.
- **Portfolio Management**: Manages user portfolios, including asset allocation and performance tracking.
- **Market Data Handling**: Retrieves and processes market data from external sources.
- **Logging**: Provides logging functionality for tracking application behavior and errors.
- **Config Management**: Handles configuration settings for the application.

## Getting Started

### Prerequisites
- C++ compiler (e.g., g++, clang++)
- CMake (for building the project)

### Installation
1. Clone the repository:
   ```
   git clone <repository-url>
   cd TradingSystem
   ```

2. Build the project using CMake:
   ```
   mkdir build
   cd build
   cmake ..
   make
   ```

### Usage
- Run the application:
   ```
   ./TradingSystem
   ```

### Running Tests
- To run the tests, navigate to the `tests` directory and execute:
   ```
   ./test_main
   ```

## Contributing
Contributions are welcome! Please feel free to submit a pull request or open an issue for any suggestions or improvements.

## License
This project is licensed under the MIT License. See the LICENSE file for more details.