@echo off
REM Complete GCP Setup and Fix Script for Windows
REM This script fixes the Cloud Build trigger creation and IAM permission issues

setlocal EnableDelayedExpansion

REM Configuration
set PROJECT_ID=tuanluongworks
set SERVICE_NAME=trading-system
set REGION=us-central1
set REPO_NAME=TradingSystem
set REPO_OWNER=tuanluongwork

echo 🔧 Complete GCP Setup and Fix for Trading System
echo ==================================================

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

REM Get project number
for /f "tokens=*" %%i in ('gcloud projects describe %PROJECT_ID% --format="value(projectNumber)"') do set PROJECT_NUMBER=%%i
echo ✅ Project: %PROJECT_ID% (Number: %PROJECT_NUMBER%)

REM Step 2: Enable all required APIs
echo.
echo 📋 Step 2: Enabling required APIs...

echo Enabling cloudbuild.googleapis.com...
gcloud services enable cloudbuild.googleapis.com --project=%PROJECT_ID%
echo Enabling run.googleapis.com...
gcloud services enable run.googleapis.com --project=%PROJECT_ID%
echo Enabling containerregistry.googleapis.com...
gcloud services enable containerregistry.googleapis.com --project=%PROJECT_ID%
echo Enabling iam.googleapis.com...
gcloud services enable iam.googleapis.com --project=%PROJECT_ID%
echo Enabling cloudresourcemanager.googleapis.com...
gcloud services enable cloudresourcemanager.googleapis.com --project=%PROJECT_ID%
echo Enabling compute.googleapis.com...
gcloud services enable compute.googleapis.com --project=%PROJECT_ID%

echo ✅ All APIs enabled

REM Step 3: Fix IAM permissions
echo.
echo 🔐 Step 3: Fixing IAM permissions...

REM Service accounts
set CLOUDBUILD_SA=%PROJECT_NUMBER%@cloudbuild.gserviceaccount.com
set COMPUTE_SA=%PROJECT_NUMBER%-compute@developer.gserviceaccount.com

echo Cloud Build SA: %CLOUDBUILD_SA%
echo Compute Engine SA: %COMPUTE_SA%

echo Assigning roles to Cloud Build service account...
gcloud projects add-iam-policy-binding %PROJECT_ID% --member="serviceAccount:%CLOUDBUILD_SA%" --role="roles/run.admin" --quiet
gcloud projects add-iam-policy-binding %PROJECT_ID% --member="serviceAccount:%CLOUDBUILD_SA%" --role="roles/iam.serviceAccountUser" --quiet
gcloud projects add-iam-policy-binding %PROJECT_ID% --member="serviceAccount:%CLOUDBUILD_SA%" --role="roles/storage.admin" --quiet
gcloud projects add-iam-policy-binding %PROJECT_ID% --member="serviceAccount:%CLOUDBUILD_SA%" --role="roles/logging.logWriter" --quiet
gcloud projects add-iam-policy-binding %PROJECT_ID% --member="serviceAccount:%CLOUDBUILD_SA%" --role="roles/cloudbuild.builds.builder" --quiet

echo Assigning roles to Compute Engine service account...
gcloud projects add-iam-policy-binding %PROJECT_ID% --member="serviceAccount:%COMPUTE_SA%" --role="roles/run.admin" --quiet
gcloud projects add-iam-policy-binding %PROJECT_ID% --member="serviceAccount:%COMPUTE_SA%" --role="roles/iam.serviceAccountUser" --quiet

echo ✅ IAM permissions configured

REM Step 4: Verify Docker and build
echo.
echo 🔨 Step 4: Testing Docker build...
docker build -t test-build . >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo ✅ Docker build successful
    docker rmi test-build >nul 2>&1
) else (
    echo ❌ Docker build failed - please check Dockerfile
    exit /b 1
)

REM Step 5: Manual Cloud Build test
echo.
echo ☁️ Step 5: Testing Cloud Build...
echo Submitting build to Cloud Build...

gcloud builds submit --config cloudbuild.yaml --project %PROJECT_ID% --region %REGION% --quiet

if %ERRORLEVEL% equ 0 (
    echo ✅ Cloud Build test successful
) else (
    echo ❌ Cloud Build test failed
    exit /b 1
)

REM Step 6: GitHub integration setup
echo.
echo 🔗 Step 6: GitHub Integration Setup
echo To complete the setup, you need to connect your GitHub repository:
echo.
echo 1. Go to: https://console.cloud.google.com/cloud-build/repositories?project=%PROJECT_ID%
echo 2. Click 'Connect Repository'
echo 3. Choose 'GitHub (Cloud Build GitHub App)'
echo 4. Authenticate and select repository: %REPO_OWNER%/%REPO_NAME%
echo.

set /p "GITHUB_CONNECTED=Have you connected the GitHub repository? (y/n): "
if /i "%GITHUB_CONNECTED%"=="y" (
    echo Creating Cloud Build trigger...
    
    REM Check if trigger already exists and delete it
    for /f %%i in ('gcloud builds triggers list --filter="name:%SERVICE_NAME%-trigger" --format="value(name)" ^| find /c /v ""') do set TRIGGER_EXISTS=%%i
    
    if not "%TRIGGER_EXISTS%"=="0" (
        echo Updating existing trigger...
        gcloud builds triggers delete %SERVICE_NAME%-trigger --quiet
    )
    
    REM Create trigger
    gcloud builds triggers create github ^
        --repo-name="%REPO_NAME%" ^
        --repo-owner="%REPO_OWNER%" ^
        --branch-pattern="main" ^
        --build-config="cloudbuild.yaml" ^
        --name="%SERVICE_NAME%-trigger" ^
        --description="Automated deployment for Trading System" ^
        --project=%PROJECT_ID%
    
    echo ✅ Cloud Build trigger created
) else (
    echo ⚠️ Skipping trigger creation. Run setup-cloud-build-trigger.bat after connecting GitHub.
)

REM Step 7: Final verification
echo.
echo 🔍 Step 7: Final verification...
echo Verifying Cloud Build service account permissions:
gcloud projects get-iam-policy %PROJECT_ID% --flatten="bindings[].members" --format="table(bindings.role)" --filter="bindings.members:%CLOUDBUILD_SA%"

echo.
echo 🎉 Setup completed successfully!
echo.
echo 📋 Summary of what was fixed:
echo • ✅ All required APIs enabled
echo • ✅ Cloud Build service account permissions fixed
echo • ✅ Compute Engine service account permissions fixed  
echo • ✅ Docker build tested successfully
echo • ✅ Cloud Build configuration tested
echo.
echo 🔗 Useful links:
echo • Cloud Build Console: https://console.cloud.google.com/cloud-build?project=%PROJECT_ID%
echo • Cloud Run Console: https://console.cloud.google.com/run?project=%PROJECT_ID%
echo • IAM Console: https://console.cloud.google.com/iam-admin/iam?project=%PROJECT_ID%
echo.
echo 💡 Next steps:
echo 1. Push changes to your main branch to trigger automatic deployment
echo 2. Monitor builds at: https://console.cloud.google.com/cloud-build/builds?project=%PROJECT_ID%

pause
