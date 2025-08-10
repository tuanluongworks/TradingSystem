# Trading System Backend

A high-performance C++ web backend for a trading system designed to support cross-platform development on Windows and Linux. It provides RESTful APIs for managing trading orders, user authentication, portfolio management, and real-time market data.

## Features

- **Order Management**: Create, cancel, and track trading orders
- **User Authentication**: JWT-based authentication system
- **Market Data**: Real-time market price simulation and retrieval
- **Portfolio Management**: Track user assets and portfolio value
- **Rate Limiting**: Protect APIs from abuse
- **Cross-Platform**: Works on Windows and Linux
- **File-Based Persistence**: Simple JSON-based data storage

## Architecture

The system follows a modular architecture with clear separation of concerns:

- **HTTP Server**: Custom HTTP server implementation with routing support
- **Controllers**: Handle API endpoints and business logic
- **Services**: Core business services (OrderManager, Portfolio, MarketData)
- **Database**: File-based JSON storage for persistence
- **Middleware**: Authentication and rate limiting
- **Utilities**: JSON parsing, JWT tokens, logging

## Project Structure

```
TradingSystem/
├── src/
│   ├── main.cpp                # Entry point of the application
│   ├── server/                 # HTTP server implementation
│   │   ├── HttpServer.cpp/h   # Core HTTP server
│   │   └── Router.cpp/h       # URL routing with parameter support
│   ├── trading/               # Trading-related functionalities
│   │   ├── OrderManager.cpp/h # Order management logic
│   │   ├── Portfolio.cpp/h    # Portfolio tracking
│   │   ├── MarketData.cpp/h   # Market data simulation
│   │   └── Types.h            # Common data structures
│   ├── api/                   # API controllers
│   │   ├── TradingController.cpp/h
│   │   └── AuthController.cpp/h
│   ├── database/              # Database management
│   │   ├── DatabaseManager.cpp/h
│   │   └── Models.cpp/h
│   ├── middleware/            # Middleware components
│   │   ├── AuthMiddleware.cpp/h
│   │   └── RateLimiter.cpp/h
│   ├── utils/                 # Utility classes
│   │   ├── Logger.cpp/h       # Logging functionality
│   │   ├── JsonParser.cpp/h   # JSON parsing
│   │   ├── JwtToken.cpp/h     # JWT token handling
│   │   └── Config.cpp/h       # Configuration management
│   └── common/                # Common types and constants
│       └── Constants.h
├── tests/                     # Unit and integration tests
├── third_party/              # Third-party libraries
├── scripts/                  # Build and test scripts
├── config/                   # Configuration files
├── data/                     # Database files (created at runtime)
├── CMakeLists.txt           # CMake configuration
└── README.md                # Project documentation
```

## Setup Instructions

### Prerequisites

- **Windows**: Visual Studio 2022 with C++ development tools
- **Linux**: GCC 9+ or Clang 10+
- CMake 3.20 or higher
- Git

### Building the Project

1. **Clone the Repository**
   ```bash
   git clone <repository-url>
   cd TradingSystem
   ```

2. **Build the Project**
   
   **Windows:**
   ```bash
   ./scripts/build.bat
   ```
   
   **Linux:**
   ```bash
   chmod +x ./scripts/build.sh
   ./scripts/build.sh
   ```

3. **Run the Application**
   ```bash
   cd build/Release  # Windows
   # or
   cd build         # Linux
   
   ./TradingSystem
   ```

   The server will start on port 8080 by default.

## API Documentation

### Base URL
```
http://localhost:8080
```

### Authentication

All protected endpoints require a JWT token in the Authorization header:
```
Authorization: Bearer <token>
```

### Endpoints

#### Health Check
```http
GET /health
```

#### Authentication

**Login**
```http
POST /api/v1/auth/login
Content-Type: application/json

{
  "username": "testuser",
  "password": "testpass"
}
```

Response:
```json
{
  "token": "eyJ...",
  "expiresIn": 3600
}
```

#### Trading

**Create Order** (Protected)
```http
POST /api/v1/orders
Authorization: Bearer <token>
Content-Type: application/json

{
  "symbol": "AAPL",
  "type": "BUY",
  "quantity": 100,
  "price": 150.0
}
```

**Get All Orders** (Protected)
```http
GET /api/v1/orders
Authorization: Bearer <token>
```

**Cancel Order** (Protected)
```http
DELETE /api/v1/orders/:orderId
Authorization: Bearer <token>
```

#### Market Data

**Get All Market Data** (Public)
```http
GET /api/v1/market-data
```

**Get Symbol Data** (Public)
```http
GET /api/v1/market-data/:symbol
```

#### Portfolio

**Get Portfolio** (Protected)
```http
GET /api/v1/portfolio
Authorization: Bearer <token>
```

## Configuration

Configuration files are located in the `config/` directory:

- `development.ini`: Development environment settings
- `production.ini`: Production environment settings

You can specify which configuration to use when starting the application:
```bash
./TradingSystem config/production.ini
```

## Development Tools

Use the consolidated development tools script for common tasks:

```bash
# Test Docker build locally
./dev-tools.sh test-docker

# Check development environment setup
./dev-tools.sh check-env

# Clean up Docker containers and images
./dev-tools.sh clean

# View application logs (requires deployment)
./dev-tools.sh logs

# Show help
./dev-tools.sh help
```

## Testing

Run the test suite:
```bash
./scripts/run_tests.sh  # Linux
# or
./scripts/run_tests.bat # Windows
```

## Deployment

The application uses **GitHub Actions** for automated CI/CD deployment to Google Cloud Run:

- **Automatic Deployment**: Every push to `main` branch triggers deployment
- **Testing**: All code is tested before deployment
- **Docker**: Application is containerized and pushed to Google Container Registry
- **Cloud Run**: Deployed as a fully managed service with auto-scaling

To deploy manually (not recommended), use the GitHub Actions workflow instead of local scripts.

## Security Features

1. **JWT Authentication**: Secure token-based authentication
2. **Rate Limiting**: Prevents API abuse (100 requests per minute by default)
3. **Input Validation**: All inputs are validated before processing
4. **CORS Support**: Configurable CORS headers for web clients

## Performance Considerations

- **Asynchronous I/O**: Non-blocking socket operations
- **Thread Pool**: Efficient request handling with worker threads
- **In-Memory Caching**: Fast data access for frequently used data
- **Connection Pooling**: Reuse of database connections

## Development

### Adding New Endpoints

1. Define the route in `main.cpp`
2. Create or update the appropriate controller
3. Add business logic to the service layer
4. Update tests

### Code Style

- Follow C++20 standards
- Use smart pointers for memory management
- Prefer RAII patterns
- Document public APIs

## Troubleshooting

### Common Issues

1. **Port Already in Use**: Change the port in configuration file
2. **Build Failures**: Ensure all prerequisites are installed
3. **Authentication Errors**: Check token expiration and format

### Logging

Logs are written to `trading_system.log` in the application directory.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Future Enhancements

- [ ] WebSocket support for real-time updates
- [ ] Database migration to PostgreSQL/MySQL
- [ ] Advanced order types (Stop-Loss, Limit)
- [ ] Risk management features
- [ ] Performance metrics and monitoring
- [ ] Docker containerization
- [ ] Kubernetes deployment scripts
- [ ] GraphQL API support