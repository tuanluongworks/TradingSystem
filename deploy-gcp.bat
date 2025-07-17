@echo off
REM Trading System GCP Deployment Script for Windows
REM Make sure you have the following prerequisites:
REM 1. Google Cloud SDK installed
REM 2. Docker Desktop installed and running
REM 3. Authenticated with GCP (gcloud auth login)

setlocal EnableDelayedExpansion

REM Configuration
set PROJECT_ID=tuanluongworks
set SERVICE_NAME=trading-system
set REGION=us-central1
set IMAGE_NAME=gcr.io/%PROJECT_ID%/%SERVICE_NAME%

echo 🚀 Starting deployment to Google Cloud Run...

REM Step 1: Enable required APIs
echo 📋 Enabling required Google Cloud APIs...
gcloud services enable run.googleapis.com --project=%PROJECT_ID%
gcloud services enable containerregistry.googleapis.com --project=%PROJECT_ID%
gcloud services enable cloudbuild.googleapis.com --project=%PROJECT_ID%

REM Step 2: Configure Docker for GCR
echo 🐳 Configuring Docker for Google Container Registry...
gcloud auth configure-docker

REM Step 3: Build the Docker image
echo 🔨 Building Docker image...
docker build -t %IMAGE_NAME%:latest .
if %ERRORLEVEL% neq 0 (
    echo ❌ Docker build failed!
    exit /b 1
)

REM Step 4: Push the image to Google Container Registry
echo 📦 Pushing image to Google Container Registry...
docker push %IMAGE_NAME%:latest
if %ERRORLEVEL% neq 0 (
    echo ❌ Docker push failed!
    exit /b 1
)

REM Step 5: Deploy to Cloud Run
echo ☁️ Deploying to Google Cloud Run...
gcloud run deploy %SERVICE_NAME% ^
    --image %IMAGE_NAME%:latest ^
    --platform managed ^
    --region %REGION% ^
    --allow-unauthenticated ^
    --memory 512Mi ^
    --cpu 1 ^
    --concurrency 100 ^
    --max-instances 10 ^
    --min-instances 0 ^
    --port 8080 ^
    --set-env-vars "PORT=8080,CONFIG_FILE=config/production.ini" ^
    --project %PROJECT_ID%

if %ERRORLEVEL% neq 0 (
    echo ❌ Cloud Run deployment failed!
    exit /b 1
)

REM Step 6: Get the service URL
echo 📋 Getting service URL...
for /f "tokens=*" %%i in ('gcloud run services describe %SERVICE_NAME% --region=%REGION% --format="value(status.url)" --project=%PROJECT_ID%') do set SERVICE_URL=%%i

echo.
echo ✅ Deployment completed successfully!
echo 🌐 Service URL: %SERVICE_URL%
echo 🔍 Health check: %SERVICE_URL%/health
echo.
echo 📊 To view logs:
echo    gcloud logs tail --project=%PROJECT_ID%
echo.
echo 📈 To view monitoring:
echo    https://console.cloud.google.com/run/detail/%REGION%/%SERVICE_NAME%/metrics?project=%PROJECT_ID%

pause
