# Multi-stage Docker build for C++ Trading System
FROM ubuntu:22.04 as builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Create build directory and build the application
RUN mkdir -p build && cd build && \
    cmake .. && \
    make -j$(nproc)

# Production stage
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libssl3 \
    && rm -rf /var/lib/apt/lists/*

# Create app directory
WORKDIR /app

# Copy the built executable from builder stage
COPY --from=builder /app/build/TradingSystem /app/
COPY --from=builder /app/config /app/config/
COPY --from=builder /app/data /app/data/

# Create logs directory
RUN mkdir -p /app/logs

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
