@echo off
REM Cloud Build Trigger Setup Script for Windows
REM Creates a GitHub-based Cloud Build trigger for continuous deployment

setlocal EnableDelayedExpansion

REM Configuration
set PROJECT_ID=tuanluongworks
set SERVICE_NAME=trading-system
set REPO_NAME=TradingSystem
set REPO_OWNER=tuanluongwork
set BRANCH_PATTERN=main

echo 🔧 Setting up Cloud Build trigger for GitHub repository...

REM Step 1: Check if already authenticated and APIs are enabled
echo 📋 Checking prerequisites...

REM Verify project
for /f "tokens=*" %%i in ('gcloud config get-value project 2^>nul') do set CURRENT_PROJECT=%%i
if not "%CURRENT_PROJECT%"=="%PROJECT_ID%" (
    echo ⚠️ Setting project to %PROJECT_ID%
    gcloud config set project %PROJECT_ID%
)

REM Step 2: Connect GitHub repository (if not already connected)
echo 🔗 Connecting GitHub repository...

REM List existing repositories
echo Checking if repository is already connected...
for /f %%i in ('gcloud builds repositories list --filter="name:projects/%PROJECT_ID%/locations/global/connections/*/repositories/%REPO_NAME%" --format="value(name)" ^| find /c /v ""') do set REPO_EXISTS=%%i

if "%REPO_EXISTS%"=="0" (
    echo 📝 Repository not connected. You'll need to connect it manually:
    echo 1. Go to: https://console.cloud.google.com/cloud-build/repositories
    echo 2. Click 'Connect Repository'
    echo 3. Select GitHub and authorize
    echo 4. Select repository: %REPO_OWNER%/%REPO_NAME%
    echo.
    echo After connecting the repository, run this script again.
    pause
)

REM Step 3: Create the Cloud Build trigger
echo 🚀 Creating Cloud Build trigger...

REM Check if trigger already exists
for /f %%i in ('gcloud builds triggers list --filter="name:%SERVICE_NAME%-trigger" --format="value(name)" ^| find /c /v ""') do set TRIGGER_EXISTS=%%i

if not "%TRIGGER_EXISTS%"=="0" (
    echo ⚠️ Trigger already exists. Updating...
    gcloud builds triggers delete %SERVICE_NAME%-trigger --quiet
)

REM Create the trigger
gcloud builds triggers create github ^
    --repo-name="%REPO_NAME%" ^
    --repo-owner="%REPO_OWNER%" ^
    --branch-pattern="%BRANCH_PATTERN%" ^
    --build-config="cloudbuild.yaml" ^
    --name="%SERVICE_NAME%-trigger" ^
    --description="Automated deployment for Trading System"

echo ✅ Cloud Build trigger created successfully!

REM Step 4: Test the trigger (optional)
echo.
set /p "TEST_BUILD=Would you like to run a test build now? (y/n): "
if /i "%TEST_BUILD%"=="y" (
    echo 🧪 Running test build...
    gcloud builds triggers run %SERVICE_NAME%-trigger --branch=%BRANCH_PATTERN%
    echo Build started. Check status at:
    echo https://console.cloud.google.com/cloud-build/builds?project=%PROJECT_ID%
)

echo.
echo 🎉 Setup completed!
echo.
echo 📋 Summary:
echo • Trigger name: %SERVICE_NAME%-trigger
echo • Repository: %REPO_OWNER%/%REPO_NAME%
echo • Branch: %BRANCH_PATTERN%
echo • Build config: cloudbuild.yaml
echo.
echo 🔗 Useful links:
echo • Cloud Build Console: https://console.cloud.google.com/cloud-build/triggers?project=%PROJECT_ID%
echo • Cloud Run Console: https://console.cloud.google.com/run?project=%PROJECT_ID%
echo.
echo 💡 Next time you push to the '%BRANCH_PATTERN%' branch, a build will automatically start!

pause
