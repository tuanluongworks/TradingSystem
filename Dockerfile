# Multi-stage Docker build for C++ Trading System
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    pkg-config \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code (excluding data directory for now)
COPY CMakeLists.txt ./
COPY src/ ./src/
COPY config/ ./config/
COPY tests/ ./tests/
COPY third_party/ ./third_party/

# Create build directory and build the application
RUN mkdir -p build && cd build && \
    cmake .. && \
    make -j$(nproc)

# Production stage
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libssl3 \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Create app directory
WORKDIR /app

# Copy the built executable from builder stage
COPY --from=builder /app/build/TradingSystem /app/

# Copy configuration files
COPY --from=builder /app/config /app/config/

# Copy data files directly from source (create directory if it doesn't exist)
RUN mkdir -p /app/data
COPY data/ /app/data/ 2>/dev/null || echo "No data directory found, creating empty one"

# Ensure data files exist with defaults if missing
RUN if [ ! -f /app/data/users.json ]; then echo '[]' > /app/data/users.json; fi && \
    if [ ! -f /app/data/orders.json ]; then echo '[]' > /app/data/orders.json; fi && \
    if [ ! -f /app/data/assets.json ]; then echo '[]' > /app/data/assets.json; fi && \
    if [ ! -f /app/data/market_data.json ]; then echo '{}' > /app/data/market_data.json; fi

# Create logs directory
RUN mkdir -p /app/logs

# Set proper permissions
RUN chmod +x /app/TradingSystem && \
    chmod -R 755 /app/config && \
    chmod -R 755 /app/data

# Set environment variables
ENV PORT=8080
ENV CONFIG_FILE=config/production.ini

# Expose port
EXPOSE 8080

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=30s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

# Run the application
CMD ["./TradingSystem", "config/production.ini"]
