# Trading System API Documentation

## Base URL
```
http://localhost:8080/api/v1
```

## Endpoints

### Health Check
Check if the server is running and healthy.

**GET** `/health`

**Response:**
```json
{
  "status": "healthy",
  "service": "TradingSystem"
}
```

### Authentication

#### Login
Authenticate a user and receive a token.

**POST** `/api/v1/auth/login`

**Request Body:**
```json
{
  "username": "testuser",
  "password": "testpass"
}
```

**Response:**
```json
{
  "token": "dummy-jwt-token",
  "expiresIn": 3600
}
```

### Trading

#### Create Order
Create a new trading order.

**POST** `/api/v1/orders`

**Request Body:**
```json
{
  "symbol": "AAPL",
  "type": "BUY",
  "quantity": 100,
  "price": 150.0
}
```

**Response:**
```json
{
  "orderId": "ORD123456",
  "status": "created"
}
```

#### Get All Orders
Retrieve all active orders.

**GET** `/api/v1/orders`

**Response:**
```json
{
  "orders": [
    {
      "id": "ORD123456",
      "symbol": "AAPL"
    }
  ]
}
```

#### Cancel Order
Cancel an existing order.

**DELETE** `/api/v1/orders/:orderId`

**Response:**
```json
{
  "success": true,
  "message": "Order cancelled successfully"
}
```

### Market Data

#### Get Market Data
Get current market data for a specific symbol.

**GET** `/api/v1/market-data/:symbol`

**Response:**
```json
{
  "symbol": "AAPL",
  "price": 150.25
}
```

#### Get All Market Data
Get current market data for all available symbols.

**GET** `/api/v1/market-data`

**Response:**
```json
{
  "success": true,
  "marketData": [
    {
      "symbol": "AAPL",
      "price": 150.25,
      "volume": 1000000
    }
  ]
}
```

### Portfolio

#### Get Portfolio
Get the user's current portfolio.

**GET** `/api/v1/portfolio`

**Response:**
```json
{
  "success": true,
  "totalValue": 8600.0,
  "assets": [
    {
      "symbol": "AAPL",
      "quantity": 10,
      "currentPrice": 150.0,
      "averageCost": 145.0
    }
  ]
}
```

## Error Responses

All endpoints may return error responses in the following format:

```json
{
  "error": "Error message description"
}
```

Common HTTP status codes:
- 200: Success
- 400: Bad Request
- 401: Unauthorized
- 404: Not Found
- 405: Method Not Allowed
- 500: Internal Server Error 