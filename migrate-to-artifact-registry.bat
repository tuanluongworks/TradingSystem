@echo off
REM Automatic migration from Container Registry to Artifact Registry
REM This script uses Google's official migration tool

setlocal enabledelayedexpansion

set PROJECT_ID=tuanluongworks

echo 🚀 Starting automatic migration from Container Registry to Artifact Registry
echo ============================================================================
echo Project: %PROJECT_ID%
echo.

REM Check if gcloud is available
where gcloud >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ❌ gcloud CLI not found
    echo Please install Google Cloud SDK: https://cloud.google.com/sdk/docs/install
    echo Or run this script in Google Cloud Shell
    exit /b 1
)

REM Check authentication
echo 📋 Checking authentication...
gcloud auth list --filter=status:ACTIVE --format="value(account)" --limit=1 >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ❌ No active gcloud authentication found
    echo Please run: gcloud auth login
    exit /b 1
)

echo ✅ Authentication verified
echo.

REM Set the project
echo 🎯 Setting project...
gcloud config set project %PROJECT_ID%
echo.

REM Enable required APIs
echo 🔌 Enabling Artifact Registry API...
gcloud services enable artifactregistry.googleapis.com
echo.

REM Run the automatic migration
echo 🔄 Starting automatic migration...
echo This will:
echo   1. Create gcr.io repositories in Artifact Registry
echo   2. Suggest and apply IAM policies
echo   3. Redirect traffic from gcr.io to Artifact Registry
echo   4. Copy all container images
echo   5. Disable request-time copying
echo.

REM Start with canary reads for safer migration
echo 📊 Phase 1: Starting with 1%% canary reads...
gcloud artifacts docker upgrade migrate --projects=%PROJECT_ID% --canary-reads=1

echo.
echo ✅ Phase 1 complete (1%% canary reads)
echo 🔍 Please verify everything is working correctly before proceeding.
echo.
echo To continue the migration manually:
echo   gcloud artifacts docker upgrade migrate --projects=%PROJECT_ID% --canary-reads=10
echo   gcloud artifacts docker upgrade migrate --projects=%PROJECT_ID% --canary-reads=100
echo   gcloud artifacts docker upgrade migrate --projects=%PROJECT_ID%

endlocal
