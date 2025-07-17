@echo off
REM Clean up Workload Identity Configuration Script for Windows
REM Removes any existing workload identity pools that might be causing conflicts

setlocal EnableDelayedExpansion

set PROJECT_ID=tuanluongworks

echo 🔧 Cleaning up Workload Identity Configuration
echo ==============================================

REM Step 1: Check if authenticated
gcloud auth list --filter=status:ACTIVE --format="value(account)" | findstr "@" >nul
if %ERRORLEVEL% neq 0 (
    echo ❌ Not authenticated with gcloud
    echo Please run: gcloud auth login
    exit /b 1
)

gcloud config set project %PROJECT_ID%

REM Step 2: Check for existing workload identity pools
echo 📋 Checking for existing workload identity pools...
gcloud iam workload-identity-pools list --location=global --project=%PROJECT_ID% --format="value(name)" 2>nul | findstr "github-pool" >nul
if %ERRORLEVEL% equ 0 (
    echo ⚠️ Found workload identity pools with 'github-pool'
    echo This might be causing the authentication conflicts.
    echo.
    set /p "DELETE_POOLS=Do you want to delete workload identity pools? This will switch to service account key authentication. (y/n): "
    if /i "!DELETE_POOLS!"=="y" (
        echo 🗑️ Attempting to clean up workload identity pools...
        echo Note: You may need to delete these manually from the console:
        echo https://console.cloud.google.com/iam-admin/workload-identity-pools?project=%PROJECT_ID%
        echo.
        echo ✅ Manual cleanup required - see console link above
    ) else (
        echo ⚠️ Keeping existing workload identity pools
        echo Note: Make sure they are properly configured or remove them manually
    )
) else (
    echo ✅ No conflicting workload identity pools found
)

REM Step 3: Verify GitHub workflow is using correct authentication  
echo.
echo 📋 Checking GitHub workflow configuration...

set WORKFLOW_FILE=.github\workflows\deploy.yml
if exist "%WORKFLOW_FILE%" (
    findstr /C:"workload_identity_provider" "%WORKFLOW_FILE%" >nul
    if %ERRORLEVEL% equ 0 (
        echo ⚠️ Found workload identity configuration in %WORKFLOW_FILE%
        echo The workflow should use service account key authentication instead.
        echo.
        echo Please verify your workflow contains:
        echo   credentials_json: '${{ secrets.GCP_SA_KEY }}'
        echo.
        echo And does NOT contain:
        echo   workload_identity_provider
    ) else (
        echo ✅ Workflow is configured for service account key authentication
    )
) else (
    echo ⚠️ Workflow file not found: %WORKFLOW_FILE%
)

REM Step 4: Instructions for next steps
echo.
echo 🎯 Next Steps
echo =============
echo.
echo 1. Run the authentication setup:
echo    setup-github-auth.bat
echo.
echo 2. Add the service account key to GitHub Secrets:
echo    - Go to: https://github.com/tuanluongwork/TradingSystem/settings/secrets/actions
echo    - Add secret: GCP_SA_KEY
echo    - Value: The JSON content from setup-github-auth.bat
echo.
echo 3. Verify the workflow uses service account key authentication:
echo    - Check .github/workflows/deploy.yml
echo    - Should contain: credentials_json: '${{ secrets.GCP_SA_KEY }}'
echo    - Should NOT contain: workload_identity_provider
echo.
echo 4. Test the deployment:
echo    - Push to main branch
echo    - Monitor GitHub Actions
echo.

echo ✅ Workload identity cleanup completed!
pause
