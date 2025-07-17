@echo off
REM GitHub Actions Authentication Diagnostic Script for Windows
REM Helps troubleshoot authentication issues and verify setup

setlocal EnableDelayedExpansion

REM Configuration
set PROJECT_ID=tuanluongworks
set SERVICE_ACCOUNT_NAME=github-actions
set SERVICE_ACCOUNT_EMAIL=%SERVICE_ACCOUNT_NAME%@%PROJECT_ID%.iam.gserviceaccount.com

echo 🔍 GitHub Actions Authentication Diagnostics
echo ==============================================

REM Step 1: Check if authenticated with gcloud
echo 📋 Step 1: Checking gcloud authentication...
gcloud auth list --filter=status:ACTIVE --format="value(account)" | findstr "@" >nul
if %ERRORLEVEL% equ 0 (
    for /f "tokens=*" %%i in ('gcloud auth list --filter=status:ACTIVE --format="value(account)" ^| findstr "@"') do set ACTIVE_ACCOUNT=%%i
    echo ✅ Authenticated as: !ACTIVE_ACCOUNT!
) else (
    echo ❌ Not authenticated with gcloud
    echo Please run: gcloud auth login
    exit /b 1
)

REM Step 2: Check project configuration
echo.
echo 📋 Step 2: Checking project configuration...
for /f "tokens=*" %%i in ('gcloud config get-value project 2^>nul') do set CURRENT_PROJECT=%%i
if "%CURRENT_PROJECT%"=="%PROJECT_ID%" (
    echo ✅ Project configured: %PROJECT_ID%
) else (
    echo ⚠️ Project mismatch. Current: %CURRENT_PROJECT%, Expected: %PROJECT_ID%
    gcloud config set project %PROJECT_ID%
    echo ✅ Project updated to: %PROJECT_ID%
)

REM Step 3: Check if service account exists
echo.
echo 📋 Step 3: Checking service account...
gcloud iam service-accounts describe %SERVICE_ACCOUNT_EMAIL% --project=%PROJECT_ID% >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo ✅ Service account exists: %SERVICE_ACCOUNT_EMAIL%
    echo    Service account details verified
) else (
    echo ❌ Service account does not exist: %SERVICE_ACCOUNT_EMAIL%
    echo Please run: setup-github-auth.bat to create it
    exit /b 1
)

REM Step 4: Check service account permissions
echo.
echo 📋 Step 4: Checking service account permissions...
echo Required roles for GitHub Actions deployment:

echo Checking roles/run.admin...
gcloud projects get-iam-policy %PROJECT_ID% --flatten="bindings[].members" --filter="bindings.role:roles/run.admin AND bindings.members:serviceAccount:%SERVICE_ACCOUNT_EMAIL%" --format="value(bindings.role)" | findstr "roles/run.admin" >nul
if %ERRORLEVEL% equ 0 (
    echo ✅ roles/run.admin
) else (
    echo ❌ roles/run.admin - MISSING
)

echo Checking roles/storage.admin...
gcloud projects get-iam-policy %PROJECT_ID% --flatten="bindings[].members" --filter="bindings.role:roles/storage.admin AND bindings.members:serviceAccount:%SERVICE_ACCOUNT_EMAIL%" --format="value(bindings.role)" | findstr "roles/storage.admin" >nul
if %ERRORLEVEL% equ 0 (
    echo ✅ roles/storage.admin
) else (
    echo ❌ roles/storage.admin - MISSING
)

echo Checking roles/iam.serviceAccountUser...
gcloud projects get-iam-policy %PROJECT_ID% --flatten="bindings[].members" --filter="bindings.role:roles/iam.serviceAccountUser AND bindings.members:serviceAccount:%SERVICE_ACCOUNT_EMAIL%" --format="value(bindings.role)" | findstr "roles/iam.serviceAccountUser" >nul
if %ERRORLEVEL% equ 0 (
    echo ✅ roles/iam.serviceAccountUser
) else (
    echo ❌ roles/iam.serviceAccountUser - MISSING
)

REM Step 5: Check required APIs
echo.
echo 📋 Step 5: Checking required APIs...

gcloud services list --enabled --filter="name:cloudbuild.googleapis.com" --format="value(name)" | findstr "cloudbuild.googleapis.com" >nul
if %ERRORLEVEL% equ 0 (
    echo ✅ cloudbuild.googleapis.com
) else (
    echo ❌ cloudbuild.googleapis.com - DISABLED
)

gcloud services list --enabled --filter="name:run.googleapis.com" --format="value(name)" | findstr "run.googleapis.com" >nul
if %ERRORLEVEL% equ 0 (
    echo ✅ run.googleapis.com
) else (
    echo ❌ run.googleapis.com - DISABLED
)

gcloud services list --enabled --filter="name:containerregistry.googleapis.com" --format="value(name)" | findstr "containerregistry.googleapis.com" >nul
if %ERRORLEVEL% equ 0 (
    echo ✅ containerregistry.googleapis.com
) else (
    echo ❌ containerregistry.googleapis.com - DISABLED
)

REM Step 6: Check for workload identity configuration
echo.
echo 📋 Step 6: Checking for conflicting workload identity configuration...
gcloud iam workload-identity-pools list --location=global --project=%PROJECT_ID% --format="value(name)" 2>nul | findstr "github-pool" >nul
if %ERRORLEVEL% equ 0 (
    echo ⚠️ Workload Identity pool 'github-pool' found
    echo    This might be causing conflicts. For simplicity, we recommend using service account keys.
) else (
    echo ✅ No conflicting workload identity pools found
)

REM Step 7: Summary and recommendations
echo.
echo 🎯 Summary and Recommendations
echo ==============================

echo.
echo If all checks above passed:
echo 1. Run 'setup-github-auth.bat' to create a fresh service account key
echo 2. Add the JSON key as 'GCP_SA_KEY' secret in GitHub repository
echo 3. Ensure you're using the main deploy.yml workflow (not workload identity)
echo.

echo If you see failures above:
echo 1. Fix any missing roles or APIs
echo 2. Re-run this diagnostic script
echo 3. Then proceed with the setup
echo.

echo GitHub Repository Secrets:
echo - Go to: https://github.com/tuanluongwork/TradingSystem/settings/secrets/actions
echo - Ensure 'GCP_SA_KEY' secret exists and contains valid JSON
echo.

echo Workflow files:
echo - Should use: .github/workflows/deploy.yml
echo - Should NOT use workload identity unless properly configured
echo.

echo 🔧 Quick fix commands:
echo # Enable APIs
echo gcloud services enable cloudbuild.googleapis.com run.googleapis.com containerregistry.googleapis.com iam.googleapis.com --project=%PROJECT_ID%
echo.
echo # Fix permissions
echo setup-github-auth.bat
echo.

echo ✅ Diagnostic completed!
pause
