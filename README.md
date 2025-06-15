# Trading System Backend

This project is a C++ web backend for a trading system designed to support cross-platform development on Windows and Linux. It provides functionalities for managing trading orders, user authentication, and retrieving market data.

## Project Structure

```
TradingSystem
├── src
│   ├── main.cpp                # Entry point of the application
│   ├── server                   # HTTP server implementation
│   ├── trading                  # Trading-related functionalities
│   ├── api                      # API controllers for trading and authentication
│   ├── database                 # Database management and models
│   ├── utils                    # Utility classes for logging and configuration
│   └── common                   # Common types and constants
├── tests                        # Unit tests for the application
├── third_party                  # Third-party libraries
├── scripts                      # Build and test scripts
├── config                       # Configuration files for different environments
├── CMakeLists.txt              # CMake configuration for building the project
├── .gitignore                   # Files to ignore in version control
└── README.md                    # Project documentation
```

## Setup Instructions

1. **Clone the Repository**
   ```bash
   git clone <repository-url>
   cd TradingSystem
   ```

2. **Build the Project**
   - For Linux:
     ```bash
     ./scripts/build.sh
     ```
   - For Windows:
     ```bash
     ./scripts/build.bat
     ```

3. **Run the Application**
   After building, you can run the application using:
   ```bash
   ./TradingSystem
   ```

4. **Run Tests**
   To run the test suite, execute:
   ```bash
   ./scripts/run_tests.sh
   ```

## Features

- **Order Management**: Create and cancel trading orders.
- **User Authentication**: Manage user login and registration.
- **Market Data Retrieval**: Fetch and process market prices.
- **Logging**: Comprehensive logging for debugging and monitoring.

## Contributing

Contributions are welcome! Please open an issue or submit a pull request for any enhancements or bug fixes.