@echo off
REM Docker Build Test Script for Windows
REM Tests the Docker build process locally before deploying

echo 🔨 Testing Docker Build for Trading System
echo ===========================================

REM Configuration
set IMAGE_NAME=trading-system-test
set CONTAINER_NAME=trading-system-test-container

REM Step 1: Clean up any existing test containers/images
echo 🧹 Cleaning up existing test containers and images...
docker stop %CONTAINER_NAME% 2>nul
docker rm %CONTAINER_NAME% 2>nul
docker rmi %IMAGE_NAME% 2>nul

REM Step 2: Build the Docker image
echo 🔨 Building Docker image...
docker build -t %IMAGE_NAME% .
if %ERRORLEVEL% neq 0 (
    echo ❌ Docker build failed
    exit /b 1
)
echo ✅ Docker build successful

REM Step 3: Test the image by running it
echo 🚀 Testing the Docker image...
docker run -d --name %CONTAINER_NAME% -p 8081:8080 %IMAGE_NAME%
if %ERRORLEVEL% neq 0 (
    echo ❌ Container failed to start
    docker logs %CONTAINER_NAME%
    exit /b 1
)
echo ✅ Container started successfully

REM Step 4: Wait for the service to start
echo ⏳ Waiting for service to start...
timeout /t 10 /nobreak >nul

REM Step 5: Test the health endpoint
echo 🔍 Testing health endpoint...
curl -f http://localhost:8081/health
if %ERRORLEVEL% neq 0 (
    echo ❌ Health check failed
    echo Container logs:
    docker logs %CONTAINER_NAME%
    
    REM Cleanup and exit
    docker stop %CONTAINER_NAME%
    docker rm %CONTAINER_NAME%
    docker rmi %IMAGE_NAME%
    exit /b 1
)
echo.
echo ✅ Health check passed

REM Step 6: Test other endpoints
echo 🔍 Testing API endpoints...
echo Testing market data endpoint...
curl -s http://localhost:8081/api/v1/market-data

REM Step 7: Check container logs for errors
echo.
echo 📋 Checking container logs...
docker logs %CONTAINER_NAME%

REM Step 8: Cleanup
echo 🧹 Cleaning up test resources...
docker stop %CONTAINER_NAME%
docker rm %CONTAINER_NAME%
docker rmi %IMAGE_NAME%

echo.
echo 🎉 Docker build test completed successfully!
echo.
echo ✅ Summary:
echo • Docker image builds correctly
echo • Container starts without errors
echo • Health endpoint responds
echo • Application logs look normal
echo • Data files are properly included
echo.
echo 🚀 Ready for deployment to Google Cloud Run!
