@echo off
REM Setup Artifact Registry for Trading System
REM This script creates the necessary Artifact Registry repository

setlocal enabledelayedexpansion

REM Configuration
if "%PROJECT_ID%"=="" set PROJECT_ID=tuanluongworks
set REGION=us-central1
set REPOSITORY_NAME=trading-system

echo 🔧 Setting up Artifact Registry for Trading System
echo ==============================================
echo Project ID: %PROJECT_ID%
echo Region: %REGION%
echo Repository: %REPOSITORY_NAME%
echo.

REM Check if gcloud is authenticated
echo 📋 Checking authentication...
gcloud auth list --filter=status:ACTIVE --format="value(account)" --limit=1 >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ❌ No active gcloud authentication found
    echo Please run: gcloud auth login
    exit /b 1
)

REM Set the project
echo 🎯 Setting project...
gcloud config set project %PROJECT_ID%
if %ERRORLEVEL% neq 0 (
    echo ❌ Failed to set project
    exit /b 1
)

REM Enable required APIs
echo 🔌 Enabling required APIs...
gcloud services enable artifactregistry.googleapis.com
gcloud services enable cloudbuild.googleapis.com
gcloud services enable run.googleapis.com

REM Create Artifact Registry repository
echo 📦 Creating Artifact Registry repository...
gcloud artifacts repositories describe %REPOSITORY_NAME% --location=%REGION% >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo ✅ Repository %REPOSITORY_NAME% already exists
) else (
    gcloud artifacts repositories create %REPOSITORY_NAME% ^
        --repository-format=docker ^
        --location=%REGION% ^
        --description="Docker repository for Trading System"
    if %ERRORLEVEL% neq 0 (
        echo ❌ Failed to create repository
        exit /b 1
    )
    echo ✅ Repository %REPOSITORY_NAME% created successfully
)

REM Configure Docker authentication
echo 🔑 Configuring Docker authentication for Artifact Registry...
gcloud auth configure-docker %REGION%-docker.pkg.dev

echo.
echo ✅ Artifact Registry setup complete!
echo.
echo 📝 Next steps:
echo 1. Build and push your image: gcloud builds submit
echo 2. Deploy to Cloud Run using the updated cloudbuild.yaml
echo.
echo 🔗 Repository URL: %REGION%-docker.pkg.dev/%PROJECT_ID%/%REPOSITORY_NAME%

endlocal
