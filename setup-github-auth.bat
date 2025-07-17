@echo off
REM GitHub Actions Service Account Setup Script for Windows
REM Fixes the authentication error by creating proper service account and credentials

setlocal EnableDelayedExpansion

REM Configuration
set PROJECT_ID=tuanluongworks
set SERVICE_ACCOUNT_NAME=github-actions
set SERVICE_ACCOUNT_EMAIL=%SERVICE_ACCOUNT_NAME%@%PROJECT_ID%.iam.gserviceaccount.com
set KEY_FILE=github-actions-key.json

echo 🔧 Setting up GitHub Actions Service Account for GCP Authentication
echo ==================================================================

REM Step 1: Verify prerequisites
echo 📋 Step 1: Verifying prerequisites...

REM Check if gcloud is authenticated
gcloud auth list --filter=status:ACTIVE --format="value(account)" | findstr "@" >nul
if %ERRORLEVEL% neq 0 (
    echo ❌ Not authenticated with Google Cloud
    echo Please run: gcloud auth login
    exit /b 1
)

REM Set project
gcloud config set project %PROJECT_ID%
echo ✅ Using project: %PROJECT_ID%

REM Step 2: Enable required APIs
echo.
echo 📋 Step 2: Enabling required APIs...
gcloud services enable iam.googleapis.com --project=%PROJECT_ID%
gcloud services enable cloudresourcemanager.googleapis.com --project=%PROJECT_ID%
gcloud services enable cloudbuild.googleapis.com --project=%PROJECT_ID%
gcloud services enable run.googleapis.com --project=%PROJECT_ID%
gcloud services enable containerregistry.googleapis.com --project=%PROJECT_ID%

echo ✅ APIs enabled

REM Step 3: Create service account (if it doesn't exist)
echo.
echo 🔐 Step 3: Creating GitHub Actions service account...

gcloud iam service-accounts describe %SERVICE_ACCOUNT_EMAIL% --project=%PROJECT_ID% >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo ⚠️ Service account %SERVICE_ACCOUNT_EMAIL% already exists
) else (
    gcloud iam service-accounts create %SERVICE_ACCOUNT_NAME% ^
        --display-name="GitHub Actions CI/CD" ^
        --description="Service account for GitHub Actions CI/CD pipeline" ^
        --project=%PROJECT_ID%
    echo ✅ Service account created: %SERVICE_ACCOUNT_EMAIL%
)

REM Step 4: Assign required roles
echo.
echo 🔐 Step 4: Assigning IAM roles...

echo   Assigning role: roles/run.admin
gcloud projects add-iam-policy-binding %PROJECT_ID% --member="serviceAccount:%SERVICE_ACCOUNT_EMAIL%" --role="roles/run.admin" --quiet

echo   Assigning role: roles/storage.admin
gcloud projects add-iam-policy-binding %PROJECT_ID% --member="serviceAccount:%SERVICE_ACCOUNT_EMAIL%" --role="roles/storage.admin" --quiet

echo   Assigning role: roles/iam.serviceAccountUser
gcloud projects add-iam-policy-binding %PROJECT_ID% --member="serviceAccount:%SERVICE_ACCOUNT_EMAIL%" --role="roles/iam.serviceAccountUser" --quiet

echo   Assigning role: roles/cloudbuild.builds.builder
gcloud projects add-iam-policy-binding %PROJECT_ID% --member="serviceAccount:%SERVICE_ACCOUNT_EMAIL%" --role="roles/cloudbuild.builds.builder" --quiet

echo   Assigning role: roles/logging.logWriter
gcloud projects add-iam-policy-binding %PROJECT_ID% --member="serviceAccount:%SERVICE_ACCOUNT_EMAIL%" --role="roles/logging.logWriter" --quiet

echo ✅ IAM roles assigned

REM Step 5: Create and download service account key
echo.
echo 🔑 Step 5: Creating service account key...

REM Remove existing key file if it exists
if exist "%KEY_FILE%" (
    del "%KEY_FILE%"
    echo Removed existing key file
)

gcloud iam service-accounts keys create %KEY_FILE% ^
    --iam-account=%SERVICE_ACCOUNT_EMAIL% ^
    --project=%PROJECT_ID%

echo ✅ Service account key created: %KEY_FILE%

REM Step 6: Display the key content for GitHub Secrets
echo.
echo 🔐 Step 6: GitHub Secrets Configuration
echo =======================================

echo.
echo 📋 Add the following secret to your GitHub repository:
echo.
echo Secret Name: GCP_SA_KEY
echo Secret Value: (copy the entire content below)
echo.
echo --- START OF SECRET VALUE ---
type %KEY_FILE%
echo.
echo --- END OF SECRET VALUE ---
echo.

REM Step 7: Instructions for adding to GitHub
echo 📝 Instructions to add the secret to GitHub:
echo.
echo 1. Go to your GitHub repository: https://github.com/tuanluongwork/TradingSystem
echo 2. Click on 'Settings' tab
echo 3. In the left sidebar, click 'Secrets and variables' → 'Actions'
echo 4. Click 'New repository secret'
echo 5. Name: GCP_SA_KEY
echo 6. Value: Copy the entire JSON content from above
echo 7. Click 'Add secret'
echo.

REM Step 8: Verify the setup
echo 🔍 Step 8: Verifying the setup...

echo Service account details:
gcloud iam service-accounts describe %SERVICE_ACCOUNT_EMAIL% --project=%PROJECT_ID%

echo.
echo Service account roles:
gcloud projects get-iam-policy %PROJECT_ID% --flatten="bindings[].members" --format="table(bindings.role)" --filter="bindings.members:serviceAccount:%SERVICE_ACCOUNT_EMAIL%"

REM Step 9: Security cleanup recommendation
echo.
echo 🔒 Step 9: Security recommendations...
echo.
echo ⚠️ IMPORTANT SECURITY NOTES:
echo 1. The key file '%KEY_FILE%' contains sensitive credentials
echo 2. After adding to GitHub Secrets, DELETE the local key file:
echo    del %KEY_FILE%
echo 3. Never commit this key file to version control
echo 4. Rotate the key regularly for security
echo.

pause

REM Optional cleanup
set /p "DELETE_KEY=Delete the local key file now? (y/n): "
if /i "%DELETE_KEY%"=="y" (
    del %KEY_FILE%
    echo ✅ Local key file deleted
) else (
    echo ⚠️ Remember to delete '%KEY_FILE%' manually after adding to GitHub
)

echo.
echo 🎉 GitHub Actions authentication setup completed!
echo.
echo 📝 Summary:
echo • Service Account: %SERVICE_ACCOUNT_EMAIL%
echo • GitHub Secret: GCP_SA_KEY (should be added to repository)
echo • Required roles: Assigned ✅
echo.
echo 🚀 Next steps:
echo 1. Ensure the GCP_SA_KEY secret is added to GitHub
echo 2. Push code to main branch to trigger deployment
echo 3. Monitor the workflow at: https://github.com/tuanluongwork/TradingSystem/actions

pause
