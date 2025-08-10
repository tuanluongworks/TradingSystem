# syntax=docker/dockerfile:1
# Multi-stage Docker build for C++ Trading System
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    libssl-dev \
    pkg-config \
    curl \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY CMakeLists.txt ./
COPY src/ ./src/
COPY config/ ./config/
COPY tests/ ./tests/
COPY third_party/ ./third_party/

# Copy data directory
COPY data/ ./data/

# Create build directory and build the application
ARG BUILD_TYPE=Release
RUN mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} .. && \
    make -j$(nproc) app_server

# Production stage
FROM ubuntu:22.04 AS runtime

# Create non-root user
RUN useradd -u 10001 -r -s /usr/sbin/nologin appuser && \
    mkdir /app && \
    chown appuser /app

# Set working directory
WORKDIR /app

# Copy the built executable from builder stage
COPY --from=builder /app/build/app_server /app/TradingSystem

# Copy configuration files
COPY --from=builder /app/config /app/config/

# Copy data files from builder stage
COPY --from=builder /app/data /app/data/

# Create logs directory
RUN mkdir -p /app/logs && \
    chown -R appuser /app/logs

# Switch to non-root user
USER appuser

# Set environment variables
ENV PORT=8080
ENV CONFIG_FILE=config/production.ini

# Expose port
EXPOSE 8080

# Health check
HEALTHCHECK --interval=30s --timeout=5s --start-period=20s --retries=3 \
    CMD curl -fsS http://localhost:8080/health || exit 1

# Run the application
ENTRYPOINT ["/app/TradingSystem", "config/production.ini"]
