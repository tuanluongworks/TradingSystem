@echo off
REM Quick deployment test script for Windows

echo 🧪 Testing Trading System Deployment...

REM Check if Docker is running
docker info >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ❌ Docker is not running. Please start Docker Desktop.
    exit /b 1
)

REM Check if gcloud is installed
where gcloud >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ❌ Google Cloud SDK is not installed.
    echo Please install it from: https://cloud.google.com/sdk/docs/install
    exit /b 1
)

REM Check if logged in to gcloud
gcloud auth list --filter=status:ACTIVE --format="value(account)" | findstr "@" >nul
if %ERRORLEVEL% neq 0 (
    echo ❌ Not logged in to Google Cloud.
    echo Please run: gcloud auth login
    exit /b 1
)

REM Test local Docker build
echo 🔨 Testing Docker build...
docker build -t trading-system-test . >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo ✅ Docker build successful
    docker rmi trading-system-test >nul 2>&1
) else (
    echo ❌ Docker build failed
    exit /b 1
)

REM Check project configuration
for /f "tokens=*" %%i in ('gcloud config get-value project 2^>nul') do set PROJECT_ID=%%i
if "%PROJECT_ID%"=="tuanluongworks" (
    echo ✅ Project configured correctly: %PROJECT_ID%
) else (
    echo ⚠️ Project ID is '%PROJECT_ID%', expected 'tuanluongworks'
    echo Run: gcloud config set project tuanluongworks
)

echo.
echo 🚀 Ready for deployment!
echo Run deploy-gcp.bat to deploy to Google Cloud Run
pause
